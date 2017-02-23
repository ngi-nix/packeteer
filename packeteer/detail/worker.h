/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 * Copyright (c) 2012-2014 Unwesen Ltd.
 * Copyright (c) 2015-2017 Jens Finkhaeuser.
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
#ifndef PACKETEER_DETAIL_WORKER_H
#define PACKETEER_DETAIL_WORKER_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer/packeteer.h>

#include <twine/tasklet.h>

#include <packeteer/concurrent_queue.h>

#include <packeteer/detail/scheduler_impl.h>

namespace packeteer {
namespace detail {

/*****************************************************************************
 * Implements a worker thread for the scheduler implementation.
 **/
class worker
  : public twine::tasklet
{
public:
  /*****************************************************************************
   * Interface
   **/
  /**
   * The worker thread sleeps waiting for an event on the condition, and wakes
   * up to check the work queue for work to execute.
   **/
  worker(twine::condition & condition, twine::recursive_mutex & mutex,
      concurrent_queue<detail::callback_entry *> & work_queue);
  ~worker();


private:

  /**
   * Sleep()s, grabs entries from the work queue, execute_callback()s them,
   * sleeps again.
   **/
  void worker_loop(twine::tasklet & tasklet, void * /* unused */);

  /**
   * Run a callback
   **/
  error_t execute_callback(detail::callback_entry * entry);

  std::atomic<bool>                             m_alive;
  concurrent_queue<detail::callback_entry *> &  m_work_queue;
};

}} // namespace packeteer::detail

#endif // guard
