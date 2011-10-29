/**
 * This file is part of packetflinger.
 *
 * Author(s): Jens Finkhaeuser <unwesen@users.sourceforge.net>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
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

namespace pd = packetflinger::duration;

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
scheduler::schedule_once(pd::usec_t const & delay, callback const & callback)
{
  detail::scheduled_callback_entry * entry
    = new detail::scheduled_callback_entry(callback, pd::now() + delay, 0, 0);
  entry->m_add = true;
  m_impl->enqueue(entry);
}



error_t
scheduler::schedule_at(pd::usec_t const & time, callback const & callback)
{
  detail::scheduled_callback_entry * entry
    = new detail::scheduled_callback_entry(callback, time, 0, 0);
  entry->m_add = true;
  m_impl->enqueue(entry);
}



error_t
scheduler::schedule(pd::usec_t const & first, pd::usec_t const & interval,
    callback const & callback)
{
  detail::scheduled_callback_entry * entry
    = new detail::scheduled_callback_entry(callback, pd::now() + first, -1,
        interval);
  entry->m_add = true;
  m_impl->enqueue(entry);
}



error_t
scheduler::schedule(pd::usec_t const & first, pd::usec_t const & interval,
    ssize_t const & count, callback const & callback)
{
  detail::scheduled_callback_entry * entry
    = new detail::scheduled_callback_entry(callback, pd::now() + first, count,
        interval);
  entry->m_add = true;
  m_impl->enqueue(entry);
}




error_t
scheduler::unschedule(callback const & callback)
{
  detail::scheduled_callback_entry * entry
    = new detail::scheduled_callback_entry(callback, 0, 0, 0);
  entry->m_add = false;
  m_impl->enqueue(entry);
}



error_t
scheduler::register_event(uint64_t events, callback const & callback)
{
  // TODO
}



error_t
scheduler::unregister_event(uint64_t events, callback const & callback)
{
  // TODO
}



error_t
scheduler::fire_events(uint64_t events)
{
  // TODO
}



} // namespace packetflinger
