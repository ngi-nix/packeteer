/**
 * This file is part of packetflinger.
 *
 * Author(s): Jens Finkhaeuser <jens@unwesen.co.uk>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 * Copyright (c) 2012-2014 Unwesen Ltd.
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
#ifndef PACKETFLINGER_DETAIL_SCHEDULER_IMPL_H
#define PACKETFLINGER_DETAIL_SCHEDULER_IMPL_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packetflinger/scheduler.h>

#include <atomic>
#include <vector>

#include <twine/thread.h>
#include <twine/chrono.h>
#include <twine/condition.h>

#include <packetflinger/types.h>
#include <packetflinger/concurrent_queue.h>
#include <packetflinger/pipe.h>

namespace packetflinger {

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

void interrupt(pipe & pipe);
void clear_interrupt(pipe & pipe);

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
// Unfortunately, function pointers - and by extension boost::function objects
// - are only EqualityComparable, so we'll not be able to sort by callback.
// That means if we want to avoid copies of callbacks, we'd have to perform
// O(N) searches through the sets of known callbacks. The implication is that
// multi_index_container's automatic uniqueness constraints cannot be made use.
// of for keeping callbacks unique.
//
// Unfortunately, it's a cost we have to take in order to effectively handle
// changing the event mask for which a callback is registered. On the other
// hand, these situations should be much, much rarer than the lookups in the
// main scheduler loop.
//
// But the different types of callbacks impose individual restrictions, too:
enum callback_type
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


}} // namespace packetflinger::detail

#include <packetflinger/detail/io_callbacks.h>
#include <packetflinger/detail/scheduled_callbacks.h>
#include <packetflinger/detail/user_defined_callbacks.h>

namespace packetflinger {

/*****************************************************************************
 * Nested class scheduler::scheduler_impl.
 *
 * The generic scheduler interface (scheduler_generic.h) needs to know what
 * this looks like, but user code certainly doesn't.
 **/
struct scheduler::scheduler_impl
{
  /***************************************************************************
   * Types
   **/
  // Events are reported with this structure.
  struct event_data
  {
    int       fd;
    events_t  events;
  };

  // Type of action to take on an item in the in-queue
  enum action_type
  {
    ACTION_ADD      = 0,
    ACTION_REMOVE   = 1,
    ACTION_TRIGGER  = 2,
  };



  /***************************************************************************
   * Generic interface
   **/
  scheduler_impl(size_t num_worker_threads);
  ~scheduler_impl();


  /**
   * Enqueue a callback_entry. The specific type must be determined by the
   * caller, and the parameters must be set by the caller.
   **/
  void enqueue(action_type action, detail::callback_entry * entry);


  /***************************************************************************
   * Implementation-specific interface
   **/
  // TODO may need to move register/unregister things to private functions,
  // because we'll need to keep track of what's registered in the generic
  // implementation.
  // OTOH consider that this'll likely be happening anyway, because we're
  // using queues for pushing stuff into the main loop and out of it.
  void register_fd(int fd, uint64_t const & events);
  void register_fds(int const * fds, size_t size, uint64_t const & events);

  void unregister_fd(int fd, uint64_t const & events);
  void unregister_fds(int const * fds, size_t size, uint64_t const & events);

  void get_events(std::vector<event_data> & events);

private:
  /***************************************************************************
   * Types
   **/
  // Entries for the in-queue
  typedef std::pair<action_type, detail::callback_entry *> in_queue_entry_t;

  // Type for temporary entry containers.
  typedef std::vector<detail::callback_entry *>   entry_list_t;

  /***************************************************************************
   * Generic private functions
   **/
  // Setup/teardown - called from ctor/dtor, so thread-safe.
  void start_main_loop();
  void stop_main_loop();
  void start_workers(size_t num_workers);
  void stop_workers(size_t num_workers);

  // Main loop
  void main_scheduler_loop(void * /* unused */);

  inline void process_in_queue(entry_list_t & triggered);
  inline void process_in_queue_io(action_type action,
      detail::io_callback_entry * entry);
  inline void process_in_queue_scheduled(action_type action,
      detail::scheduled_callback_entry * entry);
  inline void process_in_queue_user(action_type action,
      detail::user_callback_entry * entry, entry_list_t & triggered);

  inline void dispatch_scheduled_callbacks(twine::chrono::nanoseconds const & now,
      entry_list_t & to_schedule);
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
  twine::condition                m_worker_condition;
  twine::recursive_mutex          m_worker_mutex;

  // Main loop state.
  std::atomic<bool>               m_main_loop_continue;
  twine::thread                   m_main_loop_thread;
  pipe                            m_main_loop_pipe;

  // We use a weird scheme for moving things to/from the internal containers
  // defined above.
  // - There's an in-queue that scheduler's public functions write to. The
  //   inner scheduler loop will pick the queue up, and push it into the
  //   multi-index container.
  // - The scheduler then does lookups on the multi-index container because
  //   those are going to be faster than on a queue that's potentially shared
  //   with other threads. The multi-index container belongs to the main loop
  //   only.
  // - When something needs to be executed on a worker thread, there's an out-
  //   queue for such tasks that the workers query as soon as they can.
  // The scheme avoids most uses for locks (ignoring the concurrent_queue's
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

  /***************************************************************************
   * Implementation-specific data
   **/
#if defined(PACKETFLINGER_HAVE_SYS_EPOLL_H)
  int       m_epoll_fd;
#endif

};

} // namespace packetflinger

#endif // guard
