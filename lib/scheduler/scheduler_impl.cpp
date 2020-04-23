/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 * Copyright (c) 2012-2014 Unwesen Ltd.
 * Copyright (c) 2015-2020 Jens Finkhaeuser.
 *
 * This software is licensed under the terms of the GNU GPLv3 for personal,
 * educational and non-profit use. For all other uses, alternative license
 * options are available. Please contact the copyright holder for additional
 * information, stating your intended usage.
 *
 * You can find the full text of the GPLv3 in the COPYING file in this code
 * distribution.
 *
 * This software is distributed on an "AS IS" BASIS, WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.
 **/
#include <build-config.h>

#include <packeteer/scheduler.h>

#include <packeteer/connector.h>

#include "scheduler_impl.h"

#include "worker.h"

#if defined(PACKETEER_HAVE_EPOLL_CREATE1)
#include "io/epoll.h"
#endif

#if defined(PACKETEER_HAVE_SELECT)
#include "io/select.h"
#endif

#if defined(PACKETEER_HAVE_POLL)
#include "io/poll.h"
#endif

#if defined(PACKETEER_HAVE_KQUEUE)
#include "io/kqueue.h"
#endif

#if defined(PACKETEER_HAVE_IOCP)
#include "io/iocp.h"
#endif

namespace pdt = packeteer::detail;
namespace sc = std::chrono;

namespace packeteer {

/*****************************************************************************
 * Free detail functions
 **/
namespace detail {

void interrupt(connector & pipe)
{
  char buf[1] = { '\0' };
  size_t amount = 0;
  pipe.write(buf, sizeof(buf), amount);
}



void clear_interrupt(connector & pipe)
{
  char buf[1] = { '\0' };
  size_t amount = 0;
  pipe.read(buf, sizeof(buf), amount);
}

} // namespace detail






/*****************************************************************************
 * class scheduler::scheduler_impl
 **/
scheduler::scheduler_impl::scheduler_impl(std::shared_ptr<api> api,
    size_t num_worker_threads, scheduler_type type)
  : m_api{api}
  , m_num_worker_threads{num_worker_threads}
  , m_workers{}
  , m_worker_condition{}
  , m_worker_mutex{}
  , m_main_loop_continue{true}
  , m_main_loop_thread{}
  , m_main_loop_pipe{m_api, "anon://"}
  , m_in_queue{}
  , m_out_queue{}
  , m_scheduled_callbacks{}
  , m_io{nullptr}
{
  switch (type) {
    case TYPE_AUTOMATIC:
#if defined(PACKETEER_HAVE_EPOLL_CREATE1)
      m_io = new detail::io_epoll();
#elif defined(PACKETEER_HAVE_KQUEUE)
      m_io = new detail::io_kqueue();
#elif defined(PACKETEER_HAVE_IOCP)
      m_io = new detail::io_iocp();
#elif defined(PACKETEER_HAVE_POLL)
      m_io = new detail::io_poll();
#elif defined(PACKETEER_HAVE_SELECT)
      m_io = new detail::io_select();
#else
      throw exception(ERR_UNEXPECTED, "unsupported platform.");
#endif
      break;


    case TYPE_SELECT:
#if !defined(PACKETEER_HAVE_SELECT)
      throw exception(ERR_INVALID_OPTION, "select() is not supported on this platform.");
#else
      m_io = new detail::io_select();
#endif
      break;


    case TYPE_EPOLL:
#if !defined(PACKETEER_HAVE_EPOLL_CREATE1)
      throw exception(ERR_INVALID_OPTION, "epoll() is not supported on this platform.");
#else
      m_io = new detail::io_epoll();
#endif
      break;


    case TYPE_POLL:
#if !defined(PACKETEER_HAVE_POLL)
      throw exception(ERR_INVALID_OPTION, "poll() is not supported on this platform.");
#else
      m_io = new detail::io_poll();
#endif
      break;


    case TYPE_KQUEUE:
#if !defined(PACKETEER_HAVE_KQUEUE)
      throw exception(ERR_INVALID_OPTION, "kqueue() is not supported on this platform.");
#else
      m_io = new detail::io_kqueue();
#endif
      break;


    case TYPE_IOCP:
#if !defined(PACKETEER_HAVE_IOCP)
      throw exception(ERR_INVALID_OPTION, "I/O completion ports are not supported on this platform.");
#else
      m_io = new detail::io_iocp();
#endif
      break;

    default:
      throw exception(ERR_INVALID_OPTION, "unsupported scheduler type.");
  }

  if (m_num_worker_threads > 0) {
    start_main_loop();
    adjust_workers(m_num_worker_threads);
  }
}



scheduler::scheduler_impl::~scheduler_impl()
{
  adjust_workers(0);
  stop_main_loop();

  delete m_io;

  // There might be a bunch of items still in the in- and out queues.
  in_queue_entry_t item;
  while (m_in_queue.pop(item)) {
    delete item.second;
  }

  pdt::callback_entry * entry = nullptr;
  while (m_out_queue.pop(entry)) {
    delete entry;
  }
}



void
scheduler::scheduler_impl::enqueue(action_type action,
    pdt::callback_entry * entry)
{
  m_in_queue.push(std::make_pair(action, entry));
  detail::interrupt(m_main_loop_pipe);
}



void
scheduler::scheduler_impl::start_main_loop()
{
  m_main_loop_continue = true;

  error_t err = m_main_loop_pipe.connect();
  if (ERR_SUCCESS != err) {
    throw exception(err, "Could not connect main loop pipe.");
  }
  DLOG("Main loop pipe is " << m_main_loop_pipe);

  m_io->register_connector(m_main_loop_pipe,
      PEV_IO_READ | PEV_IO_ERROR | PEV_IO_CLOSE);

  m_main_loop_thread = std::thread(&scheduler_impl::main_scheduler_loop, this);
}



void
scheduler::scheduler_impl::stop_main_loop()
{
  m_main_loop_continue = false;

  detail::interrupt(m_main_loop_pipe);
  if (m_main_loop_thread.joinable()) {
    m_main_loop_thread.join();
  }

  if (m_main_loop_pipe.connected()) {
    m_io->unregister_connector(m_main_loop_pipe,
        PEV_IO_READ | PEV_IO_ERROR | PEV_IO_CLOSE);

    m_main_loop_pipe.close();
  }
}



void
scheduler::scheduler_impl::adjust_workers(size_t num_workers)
{
  size_t have = m_workers.size();

  if (num_workers < have) {
    DLOG("Decreasing worker count from " << have << " to " << num_workers << ".");
    size_t to_stop = have - num_workers;
    size_t remain = have - to_stop;

    for (size_t i = remain ; i < have ; ++i) {
      m_workers[i]->stop();
    }
    for (size_t i = remain ; i < have ; ++i) {
      m_workers[i]->wait();
      delete m_workers[i];
      m_workers[i] = nullptr;
    }
    m_workers.resize(remain);
  }
  else if (num_workers > have) {
    DLOG("Increasing worker count from " << have << " to " << num_workers << ".");
    for (size_t i = have ; i < num_workers ; ++i) {
      auto worker = new pdt::worker(m_worker_condition, m_worker_mutex, m_out_queue);
      worker->start();
      m_workers.push_back(worker);
    }
  }
}



void
scheduler::scheduler_impl::process_in_queue(entry_list_t & triggered)
{
  in_queue_entry_t item;
  while (m_in_queue.pop(item)) {
    // No callback means nothing to do.
    if (nullptr == item.second) {
      continue;
    }

    switch (item.second->m_type) {
      case pdt::CB_ENTRY_IO:
        process_in_queue_io(item.first,
            reinterpret_cast<pdt::io_callback_entry *>(item.second));
        break;

      case pdt::CB_ENTRY_SCHEDULED:
        process_in_queue_scheduled(item.first,
            reinterpret_cast<pdt::scheduled_callback_entry *>(item.second));
        break;

      case pdt::CB_ENTRY_USER:
        process_in_queue_user(item.first,
            reinterpret_cast<pdt::user_callback_entry *>(item.second),
            triggered);
        break;

      default:
        delete item.second;
        throw exception(ERR_UNEXPECTED, "Bad callback entry type, unreachable line reached");
        break;
    }
  }
}



void
scheduler::scheduler_impl::process_in_queue_io(action_type action,
    pdt::io_callback_entry * io)
{
  switch (action) {
    case ACTION_ADD:
      {
        // Add the callback for the event mask
        auto updated = m_io_callbacks.add(io);
        m_io->register_connector(updated->m_connector, updated->m_events);
      }
      break;


    case ACTION_REMOVE:
      {
        // Add the callback from the event mask
        auto updated = m_io_callbacks.remove(io);
        m_io->unregister_connector(updated->m_connector, updated->m_events);
        delete io;
      }
      break;


    case ACTION_TRIGGER:
    default:
      delete io;
      DLOG("Ignoring invalid TRIGGER action for I/O callback.");
      break;
  }
}



void
scheduler::scheduler_impl::process_in_queue_scheduled(action_type action,
    pdt::scheduled_callback_entry * scheduled)
{
  switch (action) {
    case ACTION_ADD:
      {
        // When adding, we simply add scheduled entries. It's entirely
        // possible that the same (callback, timeout) combination is added
        // multiple times, but that might be the caller's intent.
        m_scheduled_callbacks.add(scheduled);
      }
      break;


    case ACTION_REMOVE:
      {
        // When deleting, we need to delete *all* (callback, timeout)
        // combinations that match. That might not be what the caller
        // intends, but we have no way of distinguishing between them.
        m_scheduled_callbacks.remove(scheduled);
        delete scheduled;
      }
      break;


    case ACTION_TRIGGER:
    default:
      delete scheduled;
      DLOG("Ignoring invalid TRIGGER action for scheduled callback.");
      break;
  }
}



void
scheduler::scheduler_impl::process_in_queue_user(action_type action,
    pdt::user_callback_entry * entry, entry_list_t & triggered)
{
  switch (action) {
    case ACTION_ADD:
      // Add the callback/entry mask; container takes ownership
      m_user_callbacks.add(entry);
      break;


    case ACTION_REMOVE:
      // Remove the callback/entry mask
      m_user_callbacks.remove(entry);
      delete entry;
      break;


    case ACTION_TRIGGER:
      // Remember it for a later processing stage; triggered takes ownership
      triggered.push_back(entry);
      break;


    default:
      delete entry;
      throw exception(ERR_UNEXPECTED, "Bad action for user callback, unreachable line reached");
  }
}



void
scheduler::scheduler_impl::dispatch_io_callbacks(std::vector<detail::event_data> const & events,
      entry_list_t & to_schedule)
{
  DLOG("I/O callbacks");

  // Process events, and try to find a callback for each of them.
  for (auto & event : events) {
    if (m_main_loop_pipe == event.m_connector) {
      // We just got interrupted; clear the interrupt
      detail::clear_interrupt(m_main_loop_pipe);
      continue;
    }

    // Find callback(s).
    auto callbacks = m_io_callbacks.copy_matching(event.m_connector, event.m_events);
    to_schedule.insert(to_schedule.end(), callbacks.begin(), callbacks.end());
  }
}



void
scheduler::scheduler_impl::dispatch_scheduled_callbacks(
    time_point const & now, entry_list_t & to_schedule)
{
  DLOG("scheduled callbacks at: " << now.time_since_epoch().count());

  // Scheduled callbacks are due if their timeout is older than now(). That's
  // the simplest way to deal with them.
  auto range = m_scheduled_callbacks.get_timed_out(now);
  detail::scheduled_callbacks_t::list_t to_erase;
  detail::scheduled_callbacks_t::list_t to_update;

  for (auto & entry : range) {
    DLOG("scheduled callback expired at " << now.time_since_epoch().count());
    if (duration(0) == entry->m_interval) {
      // If it's a one shot event, we want to *move* it into the to_schedule
      // vector thereby granting ownership to the worker that picks it up.
      DLOG("one-shot callback, handing over to worker");
      to_schedule.push_back(entry);
      to_erase.push_back(entry);
    }
    else {
      // Depending on whether the entry gets rescheduled (more repeats) or not
      // (last invocation), we either *copy* or *move* the entry into the
      // to_schedule vector.
      DLOG("interval callback, handing over to worker & rescheduling");
      if (entry->m_count > 0) {
        --entry->m_count;
      }

      if (0 == entry->m_count) {
        // Last invocation; can *move*
        DLOG("last invocation");
        to_schedule.push_back(entry);
        to_erase.push_back(entry);
      }
      else {
        // More invocations to come; must *copy*
        to_schedule.push_back(new pdt::scheduled_callback_entry(*entry));
        to_update.push_back(entry);
      }
    }
  }

  // At this point, to_schedule contains everything that should go into the
  // out queue, but some of the entries might still be in m_scheduled_callbacks.
  // Those entries changed their timeout, though, so if we extract a new range
  // with the exact same parameters, we should arrive at the list of entries to
  // remove from m_scheduled_callbacks.
  m_scheduled_callbacks.update(to_erase, to_update);
}



void
scheduler::scheduler_impl::dispatch_user_callbacks(entry_list_t const & triggered,
    entry_list_t & to_schedule)
{
  DLOG("triggered callbacks");

  for (auto & e : triggered) {
    if (pdt::CB_ENTRY_USER != e->m_type) {
      DLOG("invalid user callback!");
      continue;
    }

    auto entry = reinterpret_cast<pdt::user_callback_entry *>(e);
    DLOG("triggered: " << entry->m_events);

    // We ignore the callback from the entry, because it's not set. However, for
    // each entry we'll have to scour the user callbacks for any callbacks that
    // may respond to the entry's events.
    auto range = m_user_callbacks.copy_matching(entry->m_events);
    to_schedule.insert(to_schedule.end(), range.begin(), range.end());

    // This was a temporary object, and we had ownership
    delete entry;
  }
}



void
scheduler::scheduler_impl::main_scheduler_loop()
{
  DLOG("CPUS: " << std::thread::hardware_concurrency());

  try {
    while (m_main_loop_continue) {
      // Timeout is *fixed*, because:
      // - I/O events will interrupt this anyway.
      // - OSX has a minimum timeout of 20 msec for *select*
      // - It would not make sense for user/scheduled callbacks to be triggered at
      //   different resolution on different platforms.
      entry_list_t to_schedule;
      wait_for_events(sc::nanoseconds(PACKETEER_EVENT_WAIT_INTERVAL_USEC),
          to_schedule);
      DLOG("Got " << to_schedule.size() << " callbacks to invoke.");

      // After callbacks of all kinds have been added to to_schedule, we can push
      // those entries to the out queue and wake workers.
      if (!to_schedule.empty()) {
        m_out_queue.push_range(to_schedule.begin(), to_schedule.end());

        // We need to interrupt the worker pipe more than once, in order to wake
        // up multiple workers. But we don't want to interrupt the pipe more than
        // there are workers or jobs, either, to avoid needless lock contention.
        std::lock_guard<std::recursive_mutex> lock(m_worker_mutex);
        size_t interrupts = std::min(to_schedule.size(), m_workers.size());
        while (interrupts--) {
          DLOG("interrupting worker pipe");
          m_worker_condition.notify_one();
        }
      }
    }
  } catch (exception const & ex) {
    EXC_LOG("Error in main loop", ex);
  } catch (std::exception const & ex) {
    EXC_LOG("Error in main loop: ", ex);
  } catch (std::string const & str) {
    ELOG("Error in main loop: " << str);
  } catch (...) {
    ELOG("Error in main loop.");
  }

  DLOG("scheduler main loop terminated.");
}



void
scheduler::scheduler_impl::wait_for_events(duration const & timeout,
    entry_list_t & result)
{
  // While processing the in-queue, we will find triggers for user-defined
  // events. We can't really execute them until we've processed the whole
  // in-queue, so we'll store them temporarily and get back to them later.
  entry_list_t triggered;
  process_in_queue(triggered);

  // Get I/O events from the subsystem
  std::vector<detail::event_data> events;
  m_io->wait_for_events(events, timeout);
  //for (auto event : events) {
  //  LOG("got events " << event.m_events << " for " << event.m_connector);
  //}

  // Process all callbacks that want to be invoked now. Since we can't have
  // workers access the same entries we may still have in our multi-index
  // containers, we'll collect callbacks into a local vector first, and add
  // those entries to the out queue later.
  // The scheduler relinquishes ownership over entries in the to_schedule
  // vector to workers.
  time_point now = clock::now();

  dispatch_io_callbacks(events, result);
  dispatch_scheduled_callbacks(now, result);
  dispatch_user_callbacks(triggered, result);
}


/*****************************************************************************
 * Free functions
 **/


error_t
execute_callback(detail::callback_entry * entry)
{
  error_t err = ERR_SUCCESS;

  switch (entry->m_type) {
    case detail::CB_ENTRY_SCHEDULED:
      err = entry->m_callback(PEV_TIMEOUT, ERR_SUCCESS, connector{}, nullptr);
      break;

    case detail::CB_ENTRY_USER:
      {
        auto user = reinterpret_cast<detail::user_callback_entry *>(entry);
        err = user->m_callback(user->m_events, ERR_SUCCESS, connector{}, nullptr);
      }
      break;

    case detail::CB_ENTRY_IO:
      {
        auto io = reinterpret_cast<detail::io_callback_entry *>(entry);
        err = io->m_callback(io->m_events, ERR_SUCCESS, io->m_connector, nullptr);
      }
      break;

    default:
      // Unknown type. Signal an error on the callback.
      err = entry->m_callback(PEV_ERROR, ERR_UNEXPECTED, connector{}, nullptr);
      break;
  }

  // Cleanup
  return err;
}

namespace {

inline error_t
handle_entry(detail::callback_entry * entry)
{
  DLOG("Thread " << std::this_thread::get_id() << " picked up entry of type: "
      << static_cast<int>(entry->m_type));
  error_t err = ERR_SUCCESS;
  try {
    err = execute_callback(entry);
    if (ERR_SUCCESS != err) {
      ELOG("Error in callback: [" << error_name(err) << "] " << error_message(err));
    }
  } catch (exception const & ex) {
    EXC_LOG("Error in callback", ex);
    err = ex.code();
  } catch (std::exception const & ex) {
    EXC_LOG("Error in callback: ", ex);
    err = ERR_UNEXPECTED;
  } catch (std::string const & str) {
    ELOG("Error in callback: " << str);
    err = ERR_UNEXPECTED;
  } catch (...) {
    ELOG("Error in callback.");
    err = ERR_UNEXPECTED;
  }

  return err;
}

} // anonymous namespace


error_t
drain_work_queue(concurrent_queue<detail::callback_entry *> & work_queue,
    bool exit_on_failure)
{
  DLOG("Starting drain.");
  detail::callback_entry * entry = nullptr;
  error_t err = ERR_SUCCESS;
  bool process = true;
  while (work_queue.pop(entry)) {
    if (process) {
      // Process the entry (it gets freed)
      err = handle_entry(entry);
    }

    // Always delete entry
    delete entry;

    // Maybe exit
    if (ERR_SUCCESS != err && exit_on_failure) {
      process = false;
    }
  }

  DLOG("Finished drain.");
  return err;
}



error_t
drain_work_queue(entry_list_t & work_queue, bool exit_on_failure)
{
  DLOG("Starting drain.");
  error_t err = ERR_SUCCESS;
  bool process = true;
  for (auto & entry : work_queue) {
    if (process) {
      err = handle_entry(entry);
    }

    // Always delete entry
    delete entry;

    // Maybe exit
    if (ERR_SUCCESS != err && exit_on_failure) {
      process = false;
    }
  }

  work_queue.clear();

  DLOG("Finished drain.");
  return err;
}


} // namespace packeteer
