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

namespace tc = twine::chrono;

namespace packetflinger {


/*****************************************************************************
 * class scheduler
 **/

scheduler::scheduler(size_t num_worker_threads)
  : m_impl(new scheduler_impl(num_worker_threads))
{
}



scheduler::~scheduler()
{
  delete m_impl;
}



error_t
scheduler::register_fd(uint64_t events, int fd, callback const & callback)
{
  // TODO
}



error_t
scheduler::unregister_fd(uint64_t events, int fd, callback const & callback)
{
  // TODO
}



error_t
scheduler::schedule_once(tc::nanoseconds const & delay, callback const & callback)
{
  auto entry = new detail::scheduled_callback_entry(callback, tc::now() + delay,
      0, tc::nanoseconds(0));
  m_impl->enqueue(scheduler_impl::ACTION_ADD, entry);

  return ERR_SUCCESS;
}



error_t
scheduler::schedule_at(tc::nanoseconds const & time, callback const & callback)
{
  auto entry = new detail::scheduled_callback_entry(callback, time, 0,
      tc::nanoseconds(0));
  m_impl->enqueue(scheduler_impl::ACTION_ADD, entry);

  return ERR_SUCCESS;
}



error_t
scheduler::schedule(tc::nanoseconds const & first, tc::nanoseconds const & interval,
    callback const & callback)
{
  auto entry = new detail::scheduled_callback_entry(callback,
      tc::now() + first, -1, interval);
  m_impl->enqueue(scheduler_impl::ACTION_ADD, entry);

  return ERR_SUCCESS;
}



error_t
scheduler::schedule(tc::nanoseconds const & first, tc::nanoseconds const & interval,
    ssize_t const & count, callback const & callback)
{
  auto entry = new detail::scheduled_callback_entry(callback, tc::now() + first,
      count, interval);
  m_impl->enqueue(scheduler_impl::ACTION_ADD, entry);

  return ERR_SUCCESS;
}




error_t
scheduler::unschedule(callback const & callback)
{
  auto entry = new detail::scheduled_callback_entry(callback, tc::nanoseconds(0),
      0, tc::nanoseconds(0));
  m_impl->enqueue(scheduler_impl::ACTION_REMOVE, entry);

  return ERR_SUCCESS;
}



error_t
scheduler::register_event(uint64_t const & events, callback const & callback)
{
  auto entry = new detail::user_callback_entry(callback, events);
  m_impl->enqueue(scheduler_impl::ACTION_ADD, entry);

  return ERR_SUCCESS;
}



error_t
scheduler::unregister_event(uint64_t const & events, callback const & callback)
{
  auto entry = new detail::user_callback_entry(callback, events);
  m_impl->enqueue(scheduler_impl::ACTION_REMOVE, entry);

  return ERR_SUCCESS;
}



error_t
scheduler::fire_events(uint64_t const & events)
{
  if (events < EV_USER) {
    return ERR_INVALID_VALUE;
  }

  auto entry = new detail::user_callback_entry(events);
  m_impl->enqueue(scheduler_impl::ACTION_TRIGGER, entry);

  return ERR_SUCCESS;
}



} // namespace packetflinger
