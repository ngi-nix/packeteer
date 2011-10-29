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
#ifndef PACKETFLINGER_SCHEDULER_H
#define PACKETFLINGER_SCHEDULER_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packetflinger/packetflinger.h>

#include <packetflinger/callback.h>
#include <packetflinger/error.h>
#include <packetflinger/duration.h>

namespace packetflinger {

/*****************************************************************************
 * The scheduler class sits at the core of the packetflinger implementation.
 *
 * It's a cross between an efficient I/O poller and a (statically sized)
 * thread pool implementation. Functions can be scheduled to run on one of the
 * worker threads at a specified time, or when a file descriptor becomes ready
 * for I/O.
 *
 * As with other thread pool implementations, you must take care to avoid
 * performing blocking or long-running tasks in callbacks or risk reducing the
 * efficiency of the scheduler as a whole.
 *
 * For I/O events, callbacks will be invoked once per file descriptor for which
 * any I/O event occurred, e.g. once for (EV_IO_READ | EV_IO_WRITE).
 * For other events, callbacks will be invoked once per event that occurred.
 **/
class scheduler
{
public:
  /***************************************************************************
   * Types
   **/
  // Event types for which callback functions can be registered. The callback
  // will be notified for which event(s) it was invoked.
  enum event_type
  {
    EV_IO_READ    = 1,      // A file descriptor is ready for reading
    EV_IO_WRITE   = 2,      // A file descriptor is ready for writing
    EV_IO_ERROR   = 4,      // A file descriptor has produced errors
    EV_IO_CLOSE   = 8,      // A file descriptor has been closed.
    EV_TIMEOUT    = 128,    // A timout has been reached that the callback was
                            // registered for.
    EV_ERROR      = 256,    // Internal scheduler error.
    EV_USER       = 32768,  // A user-defined event was fired (see below).
  };


  /***************************************************************************
   * Interface
   **/
  /**
   * Constructor. Specify the number of worker threads to start. Note that the
   * scheduler starts an additional thread internally which dispatches events.
   **/
  scheduler(size_t num_worker_threads);
  ~scheduler();



  /**
   * Register a function for the given events on the given file descriptor.
   *
   * You can pass non-I/O events here, but EV_TIMEOUT will be ignored as there
   * is no timeout value specified.
   **/
  error_t register_fd(uint64_t events, int fd, callback const & callback);



  /**
   * Stop listening to the given events on the given file descriptor. If no
   * more events are listened to, the file descriptor and callback will be
   * forgotten.
   **/
  error_t unregister_fd(uint64_t events, int fd, callback const & callback);



  /**
   * Schedule a callback:
   * - schedule_once: run the callback once after delay microseconds.
   * - schedule_at: run the callback once when time::now() reaches the given
   *    time.
   * - schedule: run the callback after first microseconds, then keep running
   *    it every interval microseconds after the first invocation.
   *    If count is zero, the effect is the same as schedule_once() or
   *    schedule_at(). If count is negative, the effect is the same as if
   *    schedule() without the count parameter is called. If count is positive,
   *    it specifies the number of times the callback should be invoked.
   **/
  error_t schedule_once(duration::usec_t const & delay,
      callback const & callback);

  error_t schedule_at(duration::usec_t const & time, callback const & callback);

  error_t schedule(duration::usec_t const & first,
      duration::usec_t const & interval, callback const & callback);

  error_t schedule(duration::usec_t const & first,
      duration::usec_t const & interval, ssize_t const & count,
      callback const & callback);



  /**
   * Unschedule a callback. Note that we do not care about the time at which
   * the callback was scheduled; that may have already passed for repeat
   * callbacks.
   **/
  error_t unschedule(callback const & callback);



  /**
   * Register a callback for the specified events. Whenever an event with one
   * of the given event types is fired, the callback is invoked.
   *
   * You can use this function to register for any type of event, but
   * registering for system (i.e. non user-defined) events is not recommended.
   * If you do, you may slow down the system considerably. Also see
   * unregister_event() below.
   *
   * User-defined events must be specified as 64 bit unsigned integer values
   * >= EV_USER.
   **/
  error_t register_event(uint64_t events, callback const & callback);



  /**
   * Unregister a callback for the specified events.
   *
   * Note that this will not distinguish between system (i.e. non user-defined)
   * events registered via register_event() and system events registered via
   * register_fd() or any of the schedule_*() functions.
   **/
  error_t unregister_event(uint64_t events, callback const & callback);



  /**
   * Fire the specified events. If you specify system events here, the function
   * will return ERR_INVALID_VALUE and not fire any events.
   **/
  error_t fire_events(uint64_t events);


private:
  // pimpl
  struct scheduler_impl;
  scheduler_impl *  m_impl;
};

} // namespace packetflinger

#endif // guard
