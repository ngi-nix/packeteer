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
#ifndef PACKETEER_SCHEDULER_WORKER_H
#define PACKETEER_SCHEDULER_WORKER_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#include <thread>

#include <liberate/concurrency/concurrent_queue.h>
#include <liberate/concurrency/tasklet.h>

#include "scheduler_impl.h"

namespace packeteer::detail {

/*****************************************************************************
 * Implements a worker thread for the scheduler implementation.
 **/
class worker
  : public liberate::concurrency::tasklet
{
public:
  /*****************************************************************************
   * Interface
   **/
  /**
   * The worker thread sleeps waiting for an event on the condition, and wakes
   * up to check the work queue for work to execute.
   **/
  worker(
      liberate::concurrency::tasklet::sleep_condition * condition,
      work_queue_t & work_queue);
  ~worker();


private:
  /**
   * Sleep()s, runs drain_work_queue() and sleeps again.
   **/
  void worker_loop(liberate::concurrency::tasklet::context & ctx);

  work_queue_t &  m_work_queue;
};

} // namespace packeteer::detail

#endif // guard
