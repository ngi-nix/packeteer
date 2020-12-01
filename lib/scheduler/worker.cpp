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
#endif

#if defined(PACKETEER_HAVE_SYS_TIME_H)
#  include <sys/time.h>
#endif

#include <errno.h>

#include <packeteer/scheduler/events.h>

namespace packeteer::detail {

using namespace std::placeholders;

worker::worker(
    liberate::concurrency::tasklet::sleep_condition * condition,
    work_queue_t & work_queue,
    scheduler_command_queue_t & command_queue)
  : liberate::concurrency::tasklet{
      std::bind(&worker::worker_loop, this, _1),
      condition
    }
  , m_work_queue(work_queue)
  , m_command_queue(command_queue)
{
}



worker::~worker()
{
}



void
worker::worker_loop(liberate::concurrency::tasklet::context & ctx)
{
  DLOG("Worker " << std::this_thread::get_id() << " started");
  do {
    DLOG("Worker " << std::this_thread::get_id() << " woke up");
    drain_work_queue(m_work_queue, false, m_command_queue);
    DLOG("Worker " << std::this_thread::get_id() << " going to sleep");
  } while (ctx.sleep());
  DLOG("Worker " << std::this_thread::get_id() << " stopped");
}



} // namespace packeteer::detail
