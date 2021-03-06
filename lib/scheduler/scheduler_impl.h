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
#ifndef PACKETEER_SCHEDULER_SCHEDULER_IMPL_H
#define PACKETEER_SCHEDULER_SCHEDULER_IMPL_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer/scheduler.h>

#include <atomic>
#include <vector>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <liberate/concurrency/concurrent_queue.h>
#include <liberate/concurrency/tasklet.h>

#include <packeteer/scheduler/types.h>
#include <packeteer/connector.h>
#include <packeteer/handle.h>

#include "../command_queue.h"

#include "io.h"

namespace packeteer {

/*****************************************************************************
 * Forward declarations
 **/
namespace detail {
class worker;
} // namespace detail


/*****************************************************************************
 * Types
 **/
namespace detail {
// We have different requirements for the different types of callback one
// can register with the scheduler, although at least two of the three share
// some properties:
//
// - The OS handles file descriptors, so we need to lookup callbacks and,
//   event masks and timeouts with the file descriptor as a key.
//   Given that this lookup happens in the scheduler's main loop, it should
//   be as fast as possible.
// - File descriptors are unique (an OS-level restriction). We don't care
//   about ordering file descriptors.
//
// We handle different callback types individually, because different
// optimizations apply for the containers that hold them.
enum callback_type : int8_t
{
  CB_ENTRY_UNKNOWN    = -1,
  CB_ENTRY_IO         = 0,
  CB_ENTRY_SCHEDULED  = 1,
  CB_ENTRY_USER       = 2,
};

struct callback_entry
{
  callback_type   m_type;
  callback        m_callback;
  time_point      m_timestamp;


  explicit callback_entry(callback_type type)
    : m_type{type}
    , m_callback{}
    , m_timestamp{}
  {
  }


  callback_entry(callback_type type, callback const & cb)
    : m_type{type}
    , m_callback{cb}
    , m_timestamp{}
  {
  }

  virtual ~callback_entry() {}
};

}} // namespace packeteer::detail

#include "callbacks/io.h"
#include "callbacks/scheduled.h"
#include "callbacks/user_defined.h"

namespace packeteer {

/*****************************************************************************
 * Types
 **/
// TODO detail
// Type for temporary entry containers.
using entry_list_t = std::vector<detail::callback_entry *>;
using work_queue_t = liberate::concurrency::concurrent_queue<
  detail::callback_entry *
>;

// Type of command for the scheduler implementation
enum command_type : int8_t
{
  CMD_ADD      = 0,
  CMD_REMOVE   = 1,
  CMD_TRIGGER  = 2,
};

// The in queue is a command queue with associated signal
using scheduler_command_queue_t = detail::command_queue_with_signal<
  command_type,
  detail::callback_entry *
>;



/*****************************************************************************
 * Nested class scheduler::scheduler_impl.
 *
 * The generic scheduler interface (scheduler_generic.h) needs to know what
 * this looks like, but user code certainly doesn't.
 **/
struct scheduler::scheduler_impl
{

  /***************************************************************************
   * Interface
   **/
  scheduler_impl(std::shared_ptr<api> api, ssize_t num_workers,
      scheduler_type type);
  ~scheduler_impl();

  /**
   * Expose command queue for the scheduler implementation.
   **/
  scheduler_command_queue_t & commands();


  /**
   * Wait for events for the given timeout, storing them in the result list.
   **/
  void wait_for_events(duration const & timeout,
      bool soft_timeout,
      entry_list_t & result);

  /**
   * Report number of workers.
   **/
  size_t num_workers() const;

  /**
   * See scheduler::set_num_workers()
   */
  void set_num_workers(ssize_t num_workers);

  /**
   * Process the current in queue.
   */
  void process_in_queue(entry_list_t & triggered);

private:
  /***************************************************************************
   * Generic private functions
   **/
  // Setup/teardown - called from ctor/dtor, so thread-safe.
  void start_main_loop();
  void stop_main_loop();

  // Starts/stops works such that the number of workers specified is reached.
  void adjust_workers(ssize_t num_workers);

  // Main loop
  void main_scheduler_loop();

  inline void process_in_queue_io(command_type command,
      detail::io_callback_entry * entry);
  inline void process_in_queue_scheduled(command_type command,
      detail::scheduled_callback_entry * entry);
  inline void process_in_queue_user(command_type command,
      detail::user_callback_entry * entry, entry_list_t & triggered);

  inline void dispatch_io_callbacks(detail::io_events const & events,
      entry_list_t & to_schedule);
  inline void dispatch_scheduled_callbacks(
      time_point const & now, entry_list_t & to_schedule);
  inline void dispatch_user_callbacks(entry_list_t const & triggered,
      entry_list_t & to_schedule);


  /***************************************************************************
   * Implementation-specific private functions
   **/
  void init_impl();
  void deinit_impl();


  /***************************************************************************
   * Generic data
   **/
  // Context
  std::shared_ptr<api>            m_api;

  // Workers
  std::atomic<ssize_t>            m_num_workers;
  std::vector<detail::worker *>   m_workers;

  liberate::concurrency::tasklet::sleep_condition m_worker_condition;

  // Main loop state.
  std::atomic<bool>               m_main_loop_continue;
  std::thread                     m_main_loop_thread;
  connector                       m_main_loop_pipe;

  // We use a weird scheme for moving things to/from the internal containers
  // defined above.
  // - There's an in-queue that scheduler's public functions write to. The
  //   inner scheduler loop will pick the queue up, and push it into the
  //   containers holding callbacks.
  // - The scheduler then does lookups on the containers because those are
  //   going to be faster than on a queue that's potentially shared with other
  //   threads. The containers belong to the main loop only.
  // - When something needs to be executed on a worker thread, there's an out-
  //   queue for such tasks that the workers query as soon as they can.
  // The scheme avoids most uses for locks on data; the only lock involved is
  // for the condition that wakes workers up (ignoring the concurrent_queue's
  // internal, relatively efficient locking).
  //
  // Any process putting an entry into either queue relinquishes ownership over
  // the entry. Any process taking an entry out of either queue takes ownership
  // over the entry.
  scheduler_command_queue_t       m_in_queue;
  work_queue_t                    m_out_queue;

  detail::io_callbacks_t          m_io_callbacks;
  detail::scheduled_callbacks_t   m_scheduled_callbacks;
  detail::user_callbacks_t        m_user_callbacks;

  // IO subsystem
  detail::io *                    m_io;
};


/*****************************************************************************
 * Free functions
 **/

/**
 * Drain a work queue, executing each entry callback.
 **/
error_t drain_work_queue(
    work_queue_t & work_queue,
    bool exit_on_failure,
    scheduler_command_queue_t & command_queue);

error_t drain_work_queue(
    entry_list_t & work_queue,
    bool exit_on_failure,
    scheduler_command_queue_t & command_queue);


} // namespace packeteer

#endif // guard
