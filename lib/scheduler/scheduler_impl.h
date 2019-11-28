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
#ifndef PACKETEER_DETAIL_SCHEDULER_IMPL_H
#define PACKETEER_DETAIL_SCHEDULER_IMPL_H

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

#include <packeteer/types.h>
#include <packeteer/scheduler/types.h>
#include <packeteer/connector.h>
#include <packeteer/handle.h>

#include "../concurrent_queue.h"
#include "io.h"

namespace packeteer {

/*****************************************************************************
 * Forward declarations
 **/
namespace detail {
class worker;
} // namespace detail


/*****************************************************************************
 * Free (detail) functions
 **/
namespace detail {

void interrupt(connector & pipe);
void clear_interrupt(connector & pipe);

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


  callback_entry(callback_type type)
    : m_type(type)
    , m_callback()
  {
  }


  callback_entry(callback_type type, callback const & cb)
    : m_type(type)
    , m_callback(cb)
  {
  }
};

// Events are reported with this structure.
struct event_data
{
  handle    m_handle;
  events_t  m_events;
};

}} // namespace packeteer::detail

#include "callbacks/io.h"
#include "callbacks/scheduled.h"
#include "callbacks/user_defined.h"

namespace packeteer {

/*****************************************************************************
 * Nested class scheduler::scheduler_impl.
 *
 * The generic scheduler interface (scheduler_generic.h) needs to know what
 * this looks like, but user code certainly doesn't.
 **/
struct scheduler::scheduler_impl
{
  // Type of action to take on an item in the in-queue
  enum action_type : int8_t
  {
    ACTION_ADD      = 0,
    ACTION_REMOVE   = 1,
    ACTION_TRIGGER  = 2,
  };

  /***************************************************************************
   * Types
   **/
  // Type for temporary entry containers.
  using entry_list_t = std::vector<detail::callback_entry *>;


  /***************************************************************************
   * Generic interface
   **/
  scheduler_impl(size_t num_worker_threads, scheduler_type type);
  ~scheduler_impl();


  /**
   * Enqueue a callback_entry. The specific type must be determined by the
   * caller, and the parameters must be set by the caller.
   **/
  void enqueue(action_type action, detail::callback_entry * entry);


  /**
   * Wait for events for the given timeout, storing them in the result list.
   **/
  void wait_for_events(duration const & timeout,
      entry_list_t & result);
private:
  /***************************************************************************
   * Types
   **/
  // Entries for the in-queue
  using in_queue_entry_t = std::pair<action_type, detail::callback_entry *>;

  /***************************************************************************
   * Generic private functions
   **/
  // Setup/teardown - called from ctor/dtor, so thread-safe.
  void start_main_loop();
  void stop_main_loop();

  // Starts/stops works such that the number of workers specified is reached.
  void adjust_workers(size_t num_workers);

  // Main loop
  void main_scheduler_loop();

  inline void process_in_queue(entry_list_t & triggered);
  inline void process_in_queue_io(action_type action,
      detail::io_callback_entry * entry);
  inline void process_in_queue_scheduled(action_type action,
      detail::scheduled_callback_entry * entry);
  inline void process_in_queue_user(action_type action,
      detail::user_callback_entry * entry, entry_list_t & triggered);

  inline void dispatch_io_callbacks(std::vector<detail::event_data> const & events,
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
  // Workers
  std::atomic<size_t>             m_num_worker_threads;
  std::vector<detail::worker *>   m_workers;
  std::condition_variable_any     m_worker_condition;
  std::recursive_mutex            m_worker_mutex;

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
  concurrent_queue<in_queue_entry_t>          m_in_queue;
  concurrent_queue<detail::callback_entry *>  m_out_queue;

  detail::io_callbacks_t                      m_io_callbacks;
  detail::scheduled_callbacks_t               m_scheduled_callbacks;
  detail::user_callbacks_t                    m_user_callbacks;

  // IO subsystem
  detail::io *                                m_io;
};

} // namespace packeteer

#endif // guard
