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
#ifndef PACKETEER_SCHEDULER_H
#define PACKETEER_SCHEDULER_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#include <packeteer/error.h>
#include <packeteer/connector.h>

#include <packeteer/scheduler/types.h>
#include <packeteer/scheduler/callback.h>
#include <packeteer/scheduler/events.h>

namespace packeteer {

/*****************************************************************************
 * The scheduler class sits at the core of the packeteer implementation.
 *
 * It's a cross between an efficient I/O poller and a (statically sized)
 * thread pool implementation. Functions can be scheduled to run on one of the
 * worker threads at a specified time, or when a connector becomes ready for I/O.
 *
 * As with other thread pool implementations, you must take care to avoid
 * performing blocking or long-running tasks in callbacks or risk reducing the
 * efficiency of the scheduler as a whole.
 *
 * For I/O events, callbacks will be invoked once per connector for which any I/O
 * event occurred, e.g. once for (PEV_IO_READ | PEV_IO_WRITE). For other events,
 * callbacks will be invoked once per event that occurred.
 **/
class PACKETEER_API scheduler
{
public:
  /***************************************************************************
   * Types
   **/
  // Implementation types. Don't use this; the automatic selection is best.
  // This is provided mainly for debugging purposes, and will be ignored on some
  // platforms.
  enum scheduler_type : int8_t
  {
    TYPE_AUTOMATIC = 0,
    TYPE_EPOLL,   // Linux
    TYPE_KQUEUE,  // BSDs, including OSX
    TYPE_POLL,    // POSIX (old)
    TYPE_SELECT,  // POSIX (newer)
    TYPE_WIN32,   // WIN32 I/O completion ports + select
  };


  /***************************************************************************
   * Interface
   **/
  /**
   * Constructor. Specify the number of worker threads to start. Note that the
   * scheduler starts an additional thread internally which dispatches events.
   *
   * If the number of worker threads is zero, no threads will be started.
   * Instead you have to call the process_events() function in your own main
   * loop. If the number of threads is negative, the hardware concurrency value
   * is used. This is typically the number of CPU cores or threads that are
   * detected.
   *
   * The scheduler object can either run automatically with worker threads, or
   * be called manually, but not both.
   *
   * May throw if the specified type is not supported. Best leave it at
   * TYPE_AUTOMATIC.
   **/
  explicit scheduler(std::shared_ptr<api> api, ssize_t num_workers = -1,
      scheduler_type type = TYPE_AUTOMATIC);

  ~scheduler();

  /**
   * Register a function for the given events on the given connector.
   *
   * You can pass non-I/O events here, but PEV_TIMEOUT will be ignored as there
   * is no timeout value specified.
   *
   * Note that PEV_IO_READ will only be registered for the read handle of a
   * connector, whereas PEV_IO_WRITE only applies to write handles. Other
   * event types may be applied to both.
   *
   * It is not recommended that you register a callback for PEV_IO_WRITE for a
   * long time. All non-blocking connectors will generally always be ready for
   * writing, which means these callbacks will be fired all the time. A similar
   * situation applies to PEV_IO_ACCEPT, which may be triggered until the
   * accept() call completes.
   *
   * It is much better to register callbacks only when you have data to write,
   * and writing is not currently possible - then unregister the callback again
   * when it's called, and all data could be written. See IO_FLAG_REPEAT below.
   *
   * You can pass any number of IO flags. The options are:
   * - IO_FLAGS_NONE: no special treatment
   * - IO_FLAGS_ONESHOT: automatically unregister the callback after it was
   *      triggered once.
   * - IO_FLAGS_REPEAT: automatically unregister the callback after it was
   *      triggered. Then automatically re-register it if the callback returned
   *      ERR_REPEAT_ACTION.
   **/
  error_t register_connector(events_t const & events, connector const & conn,
      callback const & callback, io_flags_t const & flags = IO_FLAGS_NONE);


  /**
   * Stop listening to the given events on the given connector. If no more
   * events are listened to, the connector and callback will be forgotten.
   **/
  error_t unregister_connector(events_t const & events, connector const & conn,
      callback const & callback);

  /**
   * Stop listening to the given events on the given connector without
   * specifying a callback - any callbacks are removed.
   **/
  error_t unregister_connector(events_t const & events, connector const & conn);
  error_t unregister_connectors(events_t const & events, connector const * conns,
      size_t amount);

  /**
   * Stop listening to any events on the given connector.
   **/
  error_t unregister_connector(connector const & conn);
  error_t unregister_connectors(connector const * conns, size_t amount);


  /**
   * Schedule a callback:
   * - schedule_once: run the callback once after delay.
   * - schedule_at: run the callback once when time::now() reaches the given
   *    time.
   * - schedule: run the callback after first delays, then keep running
   *    it every interval after the first invocation.
   *    If count is zero, the effect is the same as schedule_once() or
   *    schedule_at(). If count is negative, the effect is the same as if
   *    schedule() without the count parameter is called. If count is positive,
   *    it specifies the number of times the callback should be invoked.
   **/
  template <typename durationT>
  inline error_t schedule_once(durationT const & delay,
      callback const & callback)
  {
    return schedule_once(std::chrono::duration_cast<duration>(delay),
        callback);
  }

  error_t schedule_once(duration const & delay,
      callback const & callback);



  template <typename time_durationT>
  inline error_t schedule_at(clock_time_point<time_durationT> const & time,
      callback const & callback)
  {
    return schedule_at(
        std::chrono::time_point_cast<duration, clock, time_durationT>(time),
        callback);
  }

  error_t schedule_at(time_point const & time, callback const & callback);



  template <typename time_durationT, typename durationT>
  inline error_t schedule_at(clock_time_point<time_durationT> const & first,
      durationT const & interval, callback const & callback)
  {
    return schedule_at(
        std::chrono::time_point_cast<duration, clock, time_durationT>(first),
        std::chrono::duration_cast<duration>(interval),
        callback);
  }

  error_t schedule(time_point const & first, duration const & interval,
      callback const & callback);



  template <typename time_durationT, typename durationT>
  inline error_t schedule(clock_time_point<time_durationT> const & first,
      durationT const & interval, ssize_t const & count,
      callback const & callback)
  {
    return schedule(
        std::chrono::time_point_cast<duration, clock, time_durationT>(first),
        std::chrono::duration_cast<duration>(interval),
        count, callback);
  }

  error_t schedule(time_point const & first, duration const & interval,
      ssize_t const & count, callback const & callback);



  /**
   * Unschedule a callback. Note that we do not care about the time at which
   * the callback was scheduled; that may have already passed for repeat
   * callbacks.
   **/
  error_t unschedule(callback const & callback);



  /**
   * Register a callback for the specified user-defined events. Whenever an
   * event with one of the given event types is fired, the callback is invoked.
   *
   * User-defined events must be specified as 64 bit unsigned integer values
   * >= PEV_USER.
   **/
  error_t register_event(events_t const & events, callback const & callback);



  /**
   * Unregister a callback for the specified events.
   *
   * See register_event() for details on which types of events you can register
   * a callback for.
   **/
  error_t unregister_event(events_t const & events, callback const & callback);



  /**
   * Fire the specified events. If you specify system events here, the function
   * will return ERR_INVALID_VALUE and not fire any events. Any callback
   * registered for any of the specified events will be invoked as a result.
   **/
  error_t fire_events(events_t const & events);


  /**
   * Commits schedule/unschedule requests. This is useful for the worker-thread
   * mode of operation, when you want to ensure that in your current thread, a
   * callback has been set or unset at any given time.
   */
  error_t commit_callbacks();


  /**
   * Process events; waits for events until the timeout elapses, then calls any
   * registered callbacks for events that were triggered. Use this in your own
   * main loop without any worker threads.
   *
   * Returns ERR_TIMEOUT if no events occurred within the timeout value.
   * Otherwise returns ERR_SUCCESS if all callbacks finished running
   * successfully. Returns ERR_UNSUPPORTED_ACTION if the scheduler has worker
   * threads running.
   *
   * If callbacks fail, you have two options. If exit_on_failure is true, the
   * result of the first failing callback is returned. All subsequent callbacks
   * will *not* be invoked.
   *
   * If exit_on_failure is false (the default), all callbacks will be invoked.
   * The result is the result of the last failing callback.
   *
   * By default, the function treats timeouts as a hard upper limit - that is,
   * as far as the scheduler type can manage, it should return no later than
   * this timeout (some leeway is unavoidable). It is always possible for the
   * function to return early if events occurred.
   *
   * If the soft_timeout flag is provided, the timeout value is treated as a
   * minimum. If *no* event *can* occur before the timeout expires, the
   * function may wait longer. This is the default behaviour when using
   * worker threads, but is often less useful when writing your own run
   * loop.
   **/
  error_t process_events(duration const & timeout,
      bool soft_timeout = false,
      bool exit_on_failure = false);


  /**
   * Return the current number of worker threads.
   **/
  size_t num_workers() const;


  /**
   * Adjust the current number of worker threads. This is equivalent to the
   * parameter given in the constructor, and can be used to switch to/from
   * worker thread mode.
   */
  void set_num_workers(ssize_t num_workers);

private:
  // pimpl
  struct scheduler_impl;
  std::unique_ptr<scheduler_impl> m_impl;
};

} // namespace packeteer

#endif // guard
