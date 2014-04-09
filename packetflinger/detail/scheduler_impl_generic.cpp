/**
 * This file is part of packetflinger.
 *
 * Author(s): Jens Finkhaeuser <jens@unwesen.co.uk>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 * Copyright (c) 2012 Unwesen Ltd.
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
#include <packetflinger/scheduler.h>

#include <packetflinger/detail/scheduler_impl.h>
#include <packetflinger/detail/worker.h>

namespace pdt = packetflinger::detail;

namespace packetflinger {

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
scheduler::scheduler_impl::scheduler_impl(size_t num_worker_threads)
  : m_num_worker_threads(num_worker_threads)
  , m_workers()
  , m_worker_pipe()
  , m_main_loop_continue(true)
  , m_main_loop_thread()
  , m_main_loop_pipe()
  , m_in_queue()
  , m_out_queue()
  , m_scheduled_callbacks()
{
  init_impl();

  start_main_loop();
  start_workers(m_num_worker_threads);
}



scheduler::scheduler_impl::~scheduler_impl()
{
  stop_workers(m_num_worker_threads);
  stop_main_loop();

  deinit_impl();
}



void
scheduler::scheduler_impl::enqueue(action_type action,
    pdt::callback_entry * entry)
{
  m_in_queue.push(std::make_pair(action, entry));
}



void
scheduler::scheduler_impl::start_main_loop()
{
  m_main_loop_continue = true;

  register_fd(m_main_loop_pipe.get_read_fd(),
      EV_IO_READ | EV_IO_ERROR | EV_IO_CLOSE);

  m_main_loop_thread.set_func(
      twine::thread::binder<scheduler_impl, &scheduler_impl::main_scheduler_loop>::function,
      this);
}



void
scheduler::scheduler_impl::stop_main_loop()
{
  m_main_loop_continue = false;

  detail::interrupt(m_main_loop_pipe);
  m_main_loop_thread.join();

  unregister_fd(m_main_loop_pipe.get_read_fd(),
      EV_IO_READ | EV_IO_ERROR | EV_IO_CLOSE);
}



void
scheduler::scheduler_impl::start_workers(size_t num_workers)
{
  for (size_t i = m_workers.size() ; i < num_workers ; ++i) {
    m_workers.push_back(new pdt::worker(m_worker_pipe, m_out_queue));
  }
}



void
scheduler::scheduler_impl::stop_workers(size_t num_workers)
{
  size_t current = m_workers.size();
  for (size_t i = current - 1; i >= num_workers ; --i) {
    pdt::worker * worker = m_workers[i];
    m_workers.pop_back();
    delete worker; // joins
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
        // TODO don't know what to do here.
        break;
    }
  }
}



void
scheduler::scheduler_impl::process_in_queue_io(action_type action,
    pdt::io_callback_entry * io)
{
  // TODO
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
        LOG("add scheduled callback at " << scheduled->m_timeout);
        m_scheduled_callbacks.insert(scheduled);
      }
      break;


    case ACTION_REMOVE:
      {
        // When deleting, we need to delete *all* (callback, timeout)
        // combinations that match. That might not be what the caller
        // intends, but we have no way of distinguishing between them.
        LOG("remove scheduled callback");
        m_scheduled_callbacks.erase(scheduled->m_callback);
        delete scheduled;
      }
      break;


    case ACTION_TRIGGER:
    default:
      // TODO error?
      break;
  }
}



void
scheduler::scheduler_impl::process_in_queue_user(action_type action,
    pdt::user_callback_entry * user, entry_list_t & triggered)
{
  // Adding or removing events means one of two things:
  // - If the callback is already known as a callback for user events, the new
  //   event mask will be added to/subtracted from the existing one. If due to
  //   subtraction an event mask reaches zero, the entry is removed entirely.
  // - In the case of addtion, if the callback is not yet known, the entry will
  //   be added verbatim.
  // Either way, we need to find the event mask for a callback, if we can.
  auto cb_ptr = m_user_callbacks.find(user->m_callback);
  // FIXME cb_ptr is badly named

  switch (action) {
    case ACTION_ADD:
      {
        if (cb_ptr) {
          cb_ptr->m_events |= user->m_events;
        }
        else {
          m_user_callbacks.insert(user);
        }
      }
      break;


    case ACTION_REMOVE:
      {
        if (cb_ptr) {
          cb_ptr->m_events &= ~(user->m_events);

          if (0 == cb_ptr->m_events) {
            m_user_callbacks.erase(cb_ptr);
          }
        }
      }
      break;


    case ACTION_TRIGGER:
      // Remember it for a later processing stage.
      triggered.push_back(user);
      break;


    default:
      // TODO error?
      break;
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

  for (auto entry : range) {
    LOG("scheduled callback expired at " << now);
    if (0 == entry->m_count) {
      // If it's a one shot event, we want to *move* it into the to_schedule
      // vector thereby granting ownership to the worker that picks it up.
      LOG("one-shot callback, handing over to worker");
      to_schedule.push_back(entry);
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
      }
      else {
        // More invocations to come; must *copy*
        entry->m_timeout += entry->m_interval;
        to_schedule.push_back(new pdt::scheduled_callback_entry(*entry));
      }
    }
  }

  // At this point, to_schedule contains everything that should go into the
  // out queue, but some of the entries might still be in m_scheduled_callbacks.
  // Those entries changed their timeout, though, so if we extract a new range
  // with the exact same parameters, we should arrive at the list of entries to
  // remove from m_scheduled_callbacks.
  m_scheduled_callbacks.erase_timed_out(now);
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
    auto range = m_user_callbacks.get();

    for (auto candidate : range) {
      uint64_t masked = (candidate->m_events) & (entry->m_events);
      LOG("registered for: " << candidate->m_events);
      LOG("masked: " << masked);

      if (masked) {
        // The masked events were fired. Because user callbacks are never
        // automatically unscheduled, we'll need to copy the callback and send
        // on the masked events.
        auto copy = new pdt::user_callback_entry(*candidate);
        copy->m_events = masked;
        to_schedule.push_back(copy);
      }
    }
  }
}



void
scheduler::scheduler_impl::main_scheduler_loop(void * /* ignored */)
{
  LOG("CPUS: " << boost::thread::hardware_concurrency());

  while (m_main_loop_continue) {
    // While processing the in-queue, we will find triggers for user-defined
    // events. We can't really execute them until we've processed the whole
    // in-queue, so we'll store them temporarily and get back to them later.
    entry_list_t triggered;
    process_in_queue(triggered);

    // get events
    twine::chrono::sleep(twine::chrono::milliseconds(20));
    // TODO

    // Process all callbacks that want to be invoked now. Since we can't have
    // workers access the same entries we may still have in our multi-index
    // containers, we'll collect callbacks into a local vector first, and add
    // those entries to the out queue later.
    // The scheduler relinquishes ownership over entries in the to_schedule
    // vector to workers.
    twine::chrono::nanoseconds now = twine::chrono::now();
    entry_list_t to_schedule;

    // TODO
    dispatch_scheduled_callbacks(now, to_schedule);
    dispatch_user_callbacks(triggered, to_schedule);

    // After callbacks of all kinds have been added to to_schedule, we can push
    // those entries to the out queue and wake workers.
    if (!to_schedule.empty()) {
      m_out_queue.push_range(to_schedule.begin(), to_schedule.end());

      // We need to interrupt the worker pipe more than once, in order to wake
      // up multiple workers. But we don't want to interrupt the pipe more than
      // there are workers or jobs, either.
      size_t interrupts = std::min(to_schedule.size(), m_workers.size());
      while (interrupts--) {
        LOG("interrupting worker pipe");
        detail::interrupt(m_worker_pipe);
      }
    }
  }

  LOG("scheduler main loop terminated.");
}


} // namespace packetflinger
