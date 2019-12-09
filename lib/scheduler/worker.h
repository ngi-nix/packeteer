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
#ifndef PACKETEER_SCHEDULER_WORKER_H
#define PACKETEER_SCHEDULER_WORKER_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#include <thread>

#include "../concurrent_queue.h"
#include "../thread/tasklet.h"
#include "scheduler_impl.h"

namespace packeteer {
namespace detail {

/*****************************************************************************
 * Implements a worker thread for the scheduler implementation.
 **/
class worker
  : public packeteer::thread::tasklet
{
public:
  /*****************************************************************************
   * Interface
   **/
  /**
   * The worker thread sleeps waiting for an event on the condition, and wakes
   * up to check the work queue for work to execute.
   **/
  worker(std::condition_variable_any & condition, std::recursive_mutex & mutex,
      concurrent_queue<detail::callback_entry *> & work_queue);
  ~worker();

  /**
   * Run a callback
   **/
  static error_t execute_callback(detail::callback_entry * entry);


private:

  /**
   * Sleep()s, grabs entries from the work queue, execute_callback()s them,
   * sleeps again.
   **/
  void worker_loop(packeteer::thread::tasklet & tasklet, void * /* unused */);

  std::atomic<bool>                             m_alive;
  concurrent_queue<detail::callback_entry *> &  m_work_queue;
};

}} // namespace packeteer::detail

#endif // guard
