/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 * Copyright (c) 2012-2014 Unwesen Ltd.
 * Copyright (c) 2015-2019 Jens Finkhaeuser.
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

#include <stdexcept>

#include "scheduler/scheduler_impl.h"
#include "scheduler/worker.h"

namespace sc = std::chrono;

namespace packeteer {


/*****************************************************************************
 * class scheduler
 **/

scheduler::scheduler(size_t num_worker_threads,
    scheduler_type type /* = TYPE_AUTOMATIC */)
  : m_impl(new scheduler_impl(num_worker_threads, type))
{
}



scheduler::~scheduler()
{
  delete m_impl;
}



error_t
scheduler::register_handle(events_t const & events, handle const & h,
    callback const & callback)
{
  auto entry = new detail::io_callback_entry(callback, h, events);
  m_impl->enqueue(scheduler_impl::ACTION_ADD, entry);
  return ERR_SUCCESS;
}



error_t
scheduler::unregister_handle(events_t const & events, handle const & h,
    callback const & callback)
{
  auto entry = new detail::io_callback_entry(callback, h, events);
  m_impl->enqueue(scheduler_impl::ACTION_REMOVE, entry);
  return ERR_SUCCESS;
}



error_t
scheduler::schedule_once(duration const & delay, callback const & callback)
{
  auto entry = new detail::scheduled_callback_entry(callback, clock::now() + delay);
  m_impl->enqueue(scheduler_impl::ACTION_ADD, entry);

  return ERR_SUCCESS;
}



error_t
scheduler::schedule_at(time_point const & time, callback const & callback)
{
  auto entry = new detail::scheduled_callback_entry(callback, time);
  m_impl->enqueue(scheduler_impl::ACTION_ADD, entry);

  return ERR_SUCCESS;
}



error_t
scheduler::schedule(time_point const & first, duration const & interval,
    callback const & callback)
{
  auto entry = new detail::scheduled_callback_entry(callback, first, -1,
      interval);
  m_impl->enqueue(scheduler_impl::ACTION_ADD, entry);

  return ERR_SUCCESS;
}



error_t
scheduler::schedule(time_point const & first, duration const & interval,
    ssize_t const & count, callback const & callback)
{
  auto entry = new detail::scheduled_callback_entry(callback, first, count,
      interval);
  m_impl->enqueue(scheduler_impl::ACTION_ADD, entry);

  return ERR_SUCCESS;
}




error_t
scheduler::unschedule(callback const & callback)
{
  auto entry = new detail::scheduled_callback_entry(callback, time_point());
  m_impl->enqueue(scheduler_impl::ACTION_REMOVE, entry);

  return ERR_SUCCESS;
}



error_t
scheduler::register_event(events_t const & events, callback const & callback)
{
  if (events < PEV_USER) {
    return ERR_INVALID_VALUE;
  }

  auto entry = new detail::user_callback_entry(callback, events);
  m_impl->enqueue(scheduler_impl::ACTION_ADD, entry);

  return ERR_SUCCESS;
}



error_t
scheduler::unregister_event(events_t const & events, callback const & callback)
{
  auto entry = new detail::user_callback_entry(callback, events);
  m_impl->enqueue(scheduler_impl::ACTION_REMOVE, entry);

  return ERR_SUCCESS;
}



error_t
scheduler::fire_events(events_t const & events)
{
  if (events < PEV_USER) {
    return ERR_INVALID_VALUE;
  }

  auto entry = new detail::user_callback_entry(events);
  m_impl->enqueue(scheduler_impl::ACTION_TRIGGER, entry);

  return ERR_SUCCESS;
}



error_t
scheduler::process_events(duration const & timeout,
    bool exit_on_failure /* = false */)
{
  // First, get events to schedule to workers.
  scheduler_impl::entry_list_t to_schedule;
  m_impl->wait_for_events(timeout, to_schedule);
  LOG("Got " << to_schedule.size() << " callbacks to invoke.");

  if (to_schedule.empty()) {
    // No events
    return ERR_TIMEOUT;
  }

  // Then handle these events on the worker's main function.
  error_t err = ERR_SUCCESS;
  bool delete_entries = false;
  for (auto entry : to_schedule) {
    // We're in cleanup mode; this happens when exit_on_failure is true and
    // a previous entry errored out.
    if (delete_entries) {
      delete entry;
      continue;
    }

    // Execute the callback. If we don't have a success and exit_on_failure is
    // true, switch to cleanup mode.
    err = detail::worker::execute_callback(entry);
    if (ERR_SUCCESS != err && exit_on_failure) {
      delete_entries = true;
    }
  }

  // We don't have to delete entries in to_schedule: that either happened in
  // cleanup mode or in execute_callback. Similarly, the return value is set
  // appropriately at this point.
  return err;
}


} // namespace packeteer
