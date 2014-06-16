/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@unwesen.co.uk>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 * Copyright (c) 2012-2014 Unwesen Ltd.
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
#include <packeteer/scheduler.h>

#include <packeteer/detail/scheduler_impl.h>
#include <packeteer/detail/worker.h>

#if defined(PACKETEER_HAVE_EPOLL_CREATE1)
#include <packeteer/detail/io_epoll.h>
#endif

#if defined(PACKETEER_HAVE_SELECT)
#include <packeteer/detail/io_select.h>
#endif

#if defined(PACKETEER_HAVE_POLL)
#include <packeteer/detail/io_poll.h>
#endif


namespace pdt = packeteer::detail;

namespace packeteer {

/*****************************************************************************
 * Free detail functions
 **/
namespace detail {

void interrupt(pipe & pipe)
{
  char buf[1] = { '\0' };
  pipe.write(buf, sizeof(buf));
}



void clear_interrupt(pipe & pipe)
{
  char buf[1] = { '\0' };
  size_t amount = 0;
  pipe.read(buf, sizeof(buf), amount);
}

} // namespace detail






/*****************************************************************************
 * class scheduler::scheduler_impl
 **/
scheduler::scheduler_impl::scheduler_impl(size_t num_worker_threads,
    scheduler_type type)
  : m_num_worker_threads(num_worker_threads)
  , m_workers()
  , m_worker_condition()
  , m_worker_mutex()
  , m_main_loop_continue(true)
  , m_main_loop_thread()
  , m_main_loop_pipe()
  , m_in_queue()
  , m_out_queue()
  , m_scheduled_callbacks()
  , m_io(nullptr)
{
  switch (type) {
    case TYPE_AUTOMATIC:
#if defined(PACKETEER_HAVE_EPOLL_CREATE1)
      m_io = new detail::io_epoll();
#elif defined(PACKETEER_HAVE_POLL)
      m_io = new detail::io_poll();
#elif defined(PACKETEER_HAVE_SELECT)
      m_io = new detail::io_select();
#else
      throw std::runtime_error("unsupported platform");
#endif
      break;


    case TYPE_SELECT:
#if !defined(PACKETEER_HAVE_SELECT)
      throw std::runtime_error("select is not supported on this platform.");
#endif
      m_io = new detail::io_select();
      break;


    case TYPE_EPOLL:
#if !defined(PACKETEER_HAVE_EPOLL_CREATE1)
      throw std::runtime_error("epoll is not supported on this platform.");
#endif
      m_io = new detail::io_epoll();
      break;


    case TYPE_POLL:
#if !defined(PACKETEER_HAVE_POLL)
      throw std::runtime_error("poll is not supported on this platform.");
#endif
      m_io = new detail::io_poll();
      break;


    default:
      throw std::runtime_error("unsupported scheduler type");
  }

  m_io->init();

  start_main_loop();
  start_workers(m_num_worker_threads);
}



scheduler::scheduler_impl::~scheduler_impl()
{
  stop_workers(m_num_worker_threads);
  stop_main_loop();

  m_io->deinit();
  delete m_io;
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

  m_io->register_fd(m_main_loop_pipe.get_read_fd(),
      EV_IO_READ | EV_IO_ERROR | EV_IO_CLOSE);

  m_main_loop_thread.set_func(
      twine::thread::binder<scheduler_impl, &scheduler_impl::main_scheduler_loop>::function,
      this);
  m_main_loop_thread.start();
}



void
scheduler::scheduler_impl::stop_main_loop()
{
  m_main_loop_continue = false;

  detail::interrupt(m_main_loop_pipe);
  m_main_loop_thread.join();

  m_io->unregister_fd(m_main_loop_pipe.get_read_fd(),
      EV_IO_READ | EV_IO_ERROR | EV_IO_CLOSE);
}



void
scheduler::scheduler_impl::start_workers(size_t num_workers)
{
  for (size_t i = m_workers.size() ; i < num_workers ; ++i) {
    auto worker = new pdt::worker(m_worker_condition, m_worker_mutex, m_out_queue);
    worker->start();
    m_workers.push_back(worker);
  }
}



void
scheduler::scheduler_impl::stop_workers(size_t num_workers)
{
  for (auto worker : m_workers) {
    worker->stop();
  }
  for (auto worker : m_workers) {
    worker->wait();
    delete worker;
  }
}



void
scheduler::scheduler_impl::process_in_queue(entry_list_t & triggered)
{
  in_queue_entry_t item;
  while (m_in_queue.pop(item)) {
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
        m_io_callbacks.add(io);
      }
      break;


    case ACTION_REMOVE:
      {
        // Add the callback from the event mask
        m_io_callbacks.remove(io);
        delete io;
      }
      break;


    case ACTION_TRIGGER:
    default:
      LOG("Ignoring invalid TRIGGER action for I/O callback.");
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
      LOG("Ignoring invalid TRIGGER action for scheduled callback.");
      break;
  }
}



void
scheduler::scheduler_impl::process_in_queue_user(action_type action,
    pdt::user_callback_entry * entry, entry_list_t & triggered)
{
  switch (action) {
    case ACTION_ADD:
      // Add the callback/entry mask
      m_user_callbacks.add(entry);
      break;


    case ACTION_REMOVE:
      // Remove the callback/entry mask
      m_user_callbacks.remove(entry);
      break;


    case ACTION_TRIGGER:
      // Remember it for a later processing stage.
      triggered.push_back(entry);
      break;


    default:
      throw exception(ERR_UNEXPECTED, "Bad action for user callback, unreachable line reached");
  }
}



void
scheduler::scheduler_impl::dispatch_io_callbacks(std::vector<detail::event_data> const & events,
      entry_list_t & to_schedule)
{
  int own_pipe = m_main_loop_pipe.get_read_fd();

  // Process events, and try to find a callback for each of them.
  for (auto event : events) {
    if (own_pipe == event.fd) {
      // We just got interrupted; clear the interrupt
      detail::clear_interrupt(m_main_loop_pipe);
      continue;
    }

    // Find callback(s).
    auto callbacks = m_io_callbacks.copy_matching(event.fd, event.events);
    to_schedule.insert(to_schedule.end(), callbacks.begin(), callbacks.end());
  }
}



void
scheduler::scheduler_impl::dispatch_scheduled_callbacks(twine::chrono::nanoseconds const & now,
    entry_list_t & to_schedule)
{
  LOG("scheduled callbacks at: " << now);

  // Scheduled callbacks are due if their timeout is older than now(). That's
  // the simplest way to deal with them.
  auto range = m_scheduled_callbacks.get_timed_out(now);
  detail::scheduled_callbacks_t::list_t to_erase;
  detail::scheduled_callbacks_t::list_t to_update;

  for (auto entry : range) {
    LOG("scheduled callback expired at " << now);
    if (twine::chrono::nanoseconds(0) == entry->m_interval) {
      // If it's a one shot event, we want to *move* it into the to_schedule
      // vector thereby granting ownership to the worker that picks it up.
      LOG("one-shot callback, handing over to worker");
      to_schedule.push_back(entry);
      to_erase.push_back(entry);
    }
    else {
      // Depending on whether the entry gets rescheduled (more repeats) or not
      // (last invocation), we either *copy* or *move* the entry into the
      // to_schedule vector.
      LOG("interval callback, handing over to worker & rescheduling");
      if (entry->m_count > 0) {
        --entry->m_count;
      }

      if (0 == entry->m_count) {
        // Last invocation; can *move*
        LOG("last invocation");
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
  LOG("triggered callbacks");

  for (auto e : triggered) {
    if (pdt::CB_ENTRY_USER != e->m_type) {
      LOG("invalid user callback!");
      continue;
    }

    auto entry = reinterpret_cast<pdt::user_callback_entry *>(e);
    LOG("triggered: " << entry->m_events);

    // We ignore the callback from the entry, because it's not set. However, for
    // each entry we'll have to scour the user callbacks for any callbacks that
    // may respond to the entry's events.
    auto range = m_user_callbacks.copy_matching(entry->m_events);
    to_schedule.insert(to_schedule.end(), range.begin(), range.end());
  }
}



void
scheduler::scheduler_impl::main_scheduler_loop(void * /* ignored */)
{
  LOG("CPUS: " << twine::thread::hardware_concurrency());

  while (m_main_loop_continue) {
    // While processing the in-queue, we will find triggers for user-defined
    // events. We can't really execute them until we've processed the whole
    // in-queue, so we'll store them temporarily and get back to them later.
    entry_list_t triggered;
    process_in_queue(triggered);

    // Get I/O events from the subsystem
    std::vector<detail::event_data> events;
    // FIXME timeout can be platform dependent; in a further iteration, we
    // should make it dependent on whether there are items in the in-queue and/
    // (see above) or items in the callbacks to schedule (see below)
    m_io->wait_for_events(events, twine::chrono::milliseconds(20));
    for (auto event : events) {
      LOG("got events " << event.events << " for " << event.fd);
    }

    // Process all callbacks that want to be invoked now. Since we can't have
    // workers access the same entries we may still have in our multi-index
    // containers, we'll collect callbacks into a local vector first, and add
    // those entries to the out queue later.
    // The scheduler relinquishes ownership over entries in the to_schedule
    // vector to workers.
    twine::chrono::nanoseconds now = twine::chrono::now();
    entry_list_t to_schedule;

    dispatch_io_callbacks(events, to_schedule);
    dispatch_scheduled_callbacks(now, to_schedule);
    dispatch_user_callbacks(triggered, to_schedule);

    // After callbacks of all kinds have been added to to_schedule, we can push
    // those entries to the out queue and wake workers.
    if (!to_schedule.empty()) {
      m_out_queue.push_range(to_schedule.begin(), to_schedule.end());

      // We need to interrupt the worker pipe more than once, in order to wake
      // up multiple workers. But we don't want to interrupt the pipe more than
      // there are workers or jobs, either, to avoid needless lock contention.
      twine::scoped_lock<twine::recursive_mutex> lock(m_worker_mutex);
      size_t interrupts = std::min(to_schedule.size(), m_workers.size());
      while (interrupts--) {
        LOG("interrupting worker pipe");
        m_worker_condition.notify_one();
      }
    }
  }

  LOG("scheduler main loop terminated.");
}


} // namespace packeteer
