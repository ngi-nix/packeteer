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
#ifndef PACKETFLINGER_DETAIL_WORKER_H
#define PACKETFLINGER_DETAIL_WORKER_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packetflinger/packetflinger.h>

#include <boost/thread.hpp>

#include <packetflinger/concurrent_queue.h>
#include <packetflinger/pipe.h>

#include <packetflinger/detail/scheduler_impl.h>

namespace packetflinger {
namespace detail {

/*****************************************************************************
 * Implements a worker thread for the scheduler implementation.
 **/
class worker
{
public:
  /*****************************************************************************
   * Interface
   **/
  /**
   * The worker thread sleeps waiting for an event on the pipe, and wakes up to
   * check the work queue for work to execute.
   **/
  worker(pipe & pipe, concurrent_queue<detail::callback_entry *> & work_queue);
  ~worker();


  /**
   * Shut down the worker thread. If it's currently processing a callback,
   * shutdown will commence after the callback is finished.
   **/
  void shutdown();


  /**
   * Manually wake the worker thread.
   **/
  void interrupt();

private:

  /**
   * Interrupted by interrupt() above.
   **/
  void sleep();

  /**
   * Sleep()s, grabs entries from the work queue, execute_callback()s them,
   * sleeps again.
   **/
  void worker_loop();

  /**
   * Run a callback
   **/
  void execute_callback(detail::callback_entry * entry);

  std::atomic<bool>                             m_alive;
  pipe &                                        m_pipe;
  concurrent_queue<detail::callback_entry *> &  m_work_queue;

  boost::thread                                 m_thread;
};

}} // namespace packetflinger::detail

#endif // guard
