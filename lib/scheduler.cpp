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

#include <stdexcept>

#include "scheduler/scheduler_impl.h"
#include "scheduler/worker.h"

namespace sc = std::chrono;

namespace packeteer {


/*****************************************************************************
 * class scheduler
 **/

scheduler::scheduler(std::shared_ptr<api> api, ssize_t num_workers,
    scheduler_type type /* = TYPE_AUTOMATIC */)
  : m_impl{std::make_unique<scheduler_impl>(api, num_workers, type)}
{
}



scheduler::~scheduler()
{
  DLOG("Shutting down scheduler.");
  // Not default because of std::unique_ptr<> of an undeclared struct,
  // apparently.
}



error_t
scheduler::register_connector(events_t const & events, connector const & conn,
    callback const & callback)
{
  auto entry = new detail::io_callback_entry(callback, conn, events);
  m_impl->commands().enqueue(scheduler_impl::ACTION_ADD, entry);
  m_impl->commands().commit();
  return ERR_SUCCESS;
}



error_t
scheduler::unregister_connector(events_t const & events, connector const & conn,
    callback const & callback)
{
  auto entry = new detail::io_callback_entry(callback, conn, events);
  m_impl->commands().enqueue(scheduler_impl::ACTION_REMOVE, entry);
  m_impl->commands().commit();
  return ERR_SUCCESS;
}



error_t
scheduler::unregister_connector(events_t const & events, connector const & conn)
{
  auto entry = new detail::io_callback_entry(nullptr, conn, events);
  m_impl->commands().enqueue(scheduler_impl::ACTION_REMOVE, entry);
  m_impl->commands().commit();
  return ERR_SUCCESS;
}



error_t
scheduler::unregister_connectors(events_t const & events, connector const * conns,
    size_t amount)
{
  for (size_t i = 0 ; i < amount ; ++i) {
    auto entry = new detail::io_callback_entry(nullptr, conns[i], events);
    m_impl->commands().enqueue(scheduler_impl::ACTION_REMOVE, entry);
  }
  m_impl->commands().commit();
  return ERR_SUCCESS;
}




error_t
scheduler::unregister_connector(connector const & conn)
{
  auto entry = new detail::io_callback_entry(nullptr, conn, PEV_ALL_BUILTIN);
  m_impl->commands().enqueue(scheduler_impl::ACTION_REMOVE, entry);
  m_impl->commands().commit();
  return ERR_SUCCESS;
}




error_t
scheduler::unregister_connectors(connector const * conns, size_t amount)
{
  for (size_t i = 0 ; i < amount ; ++i) {
    auto entry = new detail::io_callback_entry(nullptr, conns[i], PEV_ALL_BUILTIN);
    m_impl->commands().enqueue(scheduler_impl::ACTION_REMOVE, entry);
  }
  m_impl->commands().commit();
  return ERR_SUCCESS;
}




error_t
scheduler::schedule_once(duration const & delay, callback const & callback)
{
  auto entry = new detail::scheduled_callback_entry(callback,
      clock::now() + delay);
  m_impl->commands().enqueue(scheduler_impl::ACTION_ADD, entry);
  m_impl->commands().commit();
  return ERR_SUCCESS;
}



error_t
scheduler::schedule_at(time_point const & time, callback const & callback)
{
  auto entry = new detail::scheduled_callback_entry(callback, time);
  m_impl->commands().enqueue(scheduler_impl::ACTION_ADD, entry);
  m_impl->commands().commit();
  return ERR_SUCCESS;
}



error_t
scheduler::schedule(time_point const & first, duration const & interval,
    callback const & callback)
{
  auto entry = new detail::scheduled_callback_entry(callback, first, -1,
      interval);
  m_impl->commands().enqueue(scheduler_impl::ACTION_ADD, entry);
  m_impl->commands().commit();
  return ERR_SUCCESS;
}



error_t
scheduler::schedule(time_point const & first, duration const & interval,
    ssize_t const & count, callback const & callback)
{
  auto entry = new detail::scheduled_callback_entry(callback, first, count,
      interval);
  m_impl->commands().enqueue(scheduler_impl::ACTION_ADD, entry);
  m_impl->commands().commit();
  return ERR_SUCCESS;
}




error_t
scheduler::unschedule(callback const & callback)
{
  auto entry = new detail::scheduled_callback_entry(callback, time_point());
  m_impl->commands().enqueue(scheduler_impl::ACTION_REMOVE, entry);
  m_impl->commands().commit();
  return ERR_SUCCESS;
}



error_t
scheduler::register_event(events_t const & events, callback const & callback)
{
  if (events < PEV_USER) {
    return ERR_INVALID_VALUE;
  }

  auto entry = new detail::user_callback_entry(callback, events);
  m_impl->commands().enqueue(scheduler_impl::ACTION_ADD, entry);
  m_impl->commands().commit();
  return ERR_SUCCESS;
}



error_t
scheduler::unregister_event(events_t const & events, callback const & callback)
{
  auto entry = new detail::user_callback_entry(callback, events);
  m_impl->commands().enqueue(scheduler_impl::ACTION_REMOVE, entry);
  m_impl->commands().commit();
  return ERR_SUCCESS;
}



error_t
scheduler::fire_events(events_t const & events)
{
  if (events < PEV_USER) {
    return ERR_INVALID_VALUE;
  }

  auto entry = new detail::user_callback_entry(events);
  m_impl->commands().enqueue(scheduler_impl::ACTION_TRIGGER, entry);
  m_impl->commands().commit();
  return ERR_SUCCESS;
}



error_t
scheduler::commit_callbacks()
{
  // Since we can't process the in queue without collecting triggered events,
  // but we don't want to schedule anything here, we'll just re-push the
  // triggered events back onto the queue. It may cause some delay if the queue
  // is full, but let's take it for now.
  entry_list_t triggered;
  m_impl->process_in_queue(triggered);

  for (auto entry : triggered) {
    m_impl->commands().enqueue(scheduler_impl::ACTION_TRIGGER, entry);
  }
  m_impl->commands().commit();

  // Not much else to do?
  return ERR_SUCCESS;
}



error_t
scheduler::process_events(duration const & timeout,
    bool soft_timeout /* = false */,
    bool exit_on_failure /* = false */)
{
  if (m_impl->num_workers() > 0) {
    return ERR_UNSUPPORTED_ACTION;
  }

  // First, get events to schedule to workers.
  entry_list_t to_schedule;
  m_impl->wait_for_events(timeout, soft_timeout, to_schedule);

  if (to_schedule.empty()) {
    // No events
    return ERR_TIMEOUT;
  }
  DLOG("Got " << to_schedule.size() << " callbacks to invoke.");

  // Then handle these events on the worker's main function.
  return drain_work_queue(to_schedule, exit_on_failure);
}



size_t
scheduler::num_workers() const
{
  return m_impl->num_workers();
}



void
scheduler::set_num_workers(ssize_t num_workers)
{
  m_impl->set_num_workers(num_workers);
}


} // namespace packeteer
