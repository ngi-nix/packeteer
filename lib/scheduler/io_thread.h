/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2020 Jens Finkhaeuser.
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
#ifndef PACKETEER_SCHEDULER_IO_THREAD_H
#define PACKETEER_SCHEDULER_IO_THREAD_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#include <build-config.h>

#include <thread>

#include <liberate/concurrency/concurrent_queue.h>

#include "io.h"

namespace packeteer::detail {

/**
 * io_thread runs an I/O subsystem in a background thread, yielding
 * event_data whenever the I/O loop registers any.
 *
 * In order to do its job, it needs to have:
 * a) An I/O subsystem instance to use for the I/O loop.
 * b) A connector on which the I/O loop can be interrupted for graceful
 *    shutdown.
 * c) A queue in which to place I/O event data.
 * d) A connector on which it can notify that I/O event data is in the queue.
 *
 * Note: the queue is a little weird because we want to avoid copying
 *       event_data unnecessarily. For performance reasons, it is a vector
 *       in the io interface. In the thread model, we would have to lock a
 *       vector to append to it, introducing an unwanted bottleneck. So as
 *       a compromise, the queue is a queue of vectors of event_data.
 **/

using out_queue_t = liberate::concurrency::concurrent_queue<io_events>;

class PACKETEER_PRIVATE io_thread
{
public:
  /**
   * Type(s) for the internal connector registration/deregistration queue.
   */
  enum register_action : uint8_t
  {
    REGISTER = 0,
    UNREGISTER = 1,
  };

  struct register_item
  {
    register_action action;
    connector       conn;
    events_t        events;
  };

  using registration_queue_t = liberate::concurrency::concurrent_queue<
    register_item
  >;

  /**
   * Starts the thread with the given parameters.
   *
   * If report_self is true, then events on the io_interrupt are
   * also reported.
   */
  io_thread(std::unique_ptr<io> io, connector io_interrupt,
      out_queue_t & out_queue,
      connector queue_interrupt,
      bool report_self = false);

  ~io_thread();

  /**
   * Starts the thread.
   */
  error_t start();

  /**
   * Wakes the thread up.
   */
  void wakeup();

  /**
   * Stops the thread, and waits for it to finish.
   */
  void stop();

  /**
   * Return true if the thread is running, false otherwise.
   */
  bool is_running() const;

  /**
   * Return an error instance if the thread ended with error,
   * or nullptr.
   */
  std::exception_ptr error() const;

  /**
   * Register/unregister connector(s) for events.
   */
  void register_connector(connector const & conn, events_t events);
  void register_connectors(connector const * conns, size_t amount, events_t events);

  void unregister_connector(connector const & conn, events_t events);
  void unregister_connectors(connector const * conns, size_t amount, events_t events);

private:

  void thread_loop();

  std::unique_ptr<io> m_io;
  connector           m_io_interrupt;
  out_queue_t &       m_out_queue;
  connector           m_queue_interrupt;
  bool                m_report_self;

  registration_queue_t        m_registration_queue;

  volatile bool               m_running = true;
  std::thread                 m_thread;
  mutable std::exception_ptr  m_error = nullptr;
};


} // namespace packeteer::detail

#endif // guard
