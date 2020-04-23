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

#include "worker.h"

#if defined(PACKETEER_HAVE_SYS_SELECT_H)
#  include <sys/select.h>
#elif defined(PACKETEER_HAVE_SYS_TYPES_H) && defined(PACKETEER_HAVE_UNISTD_H)
#  include <sys/types.h>
#  include <unistd.h>
#else
// FIXME #  error Cannot compile on this system.
#endif

#if defined(PACKETEER_HAVE_SYS_TIME_H)
#  include <sys/time.h>
#endif

#include <errno.h>

#include <packeteer/scheduler/events.h>

namespace packeteer::detail {

worker::worker(std::condition_variable_any & condition, std::recursive_mutex & mutex,
    concurrent_queue<detail::callback_entry *> & work_queue)
  : packeteer::thread::tasklet(&condition, &mutex, packeteer::thread::binder(this, &worker::worker_loop))
  , m_work_queue(work_queue)
{
}



worker::~worker()
{
  stop();
}



void
worker::worker_loop(packeteer::thread::tasklet & _1 [[maybe_unused]], void * _2 [[maybe_unused]])
{
  DLOG("Worker " << std::this_thread::get_id() << " started");
  do {
    DLOG("Worker " << std::this_thread::get_id() << " woke up");
    drain_work_queue(m_work_queue, false);
    DLOG("Worker " << std::this_thread::get_id() << " going to sleep");
  } while (packeteer::thread::tasklet::sleep());
  DLOG("Worker " << std::this_thread::get_id() << " stopped");
}



} // namespace packeteer::detail
