/**
 * This file is part of packetflinger.
 *
 * Author(s): Jens Finkhaeuser <jens@unwesen.co.uk>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 * Copyright (c) 2012 Unwesen Ltd.
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

#include <boost/thread.hpp>

#include <boost/functional/hash.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/member.hpp>

#include <packetflinger/duration.h>
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

//
// 1. I/O callbacks:
//
//  - While the main scheduler loop will look up metadata with a file
//    descriptor key, the value in this case is a (callback, eventmask)
//    tuple.
//    The above comments on callback uniqueness hold.
//  - We do not care about the ordering of (callback, eventmask).
//  - (callback, eventmask) needs to be modifyable, as users can register
//    and unregister multiple events for the same (callback, FD) tuple.

// Tags
struct fd_tag {};
struct events_tag {};

struct io_callback_entry : public callback_entry
{
  int           m_fd;
  uint64_t      m_events;

  io_callback_entry()
    : callback_entry(CB_ENTRY_IO)
    , m_fd()
    , m_events()
  {
  }
};

typedef boost::multi_index_container<
  io_callback_entry *,

  boost::multi_index::indexed_by<
    // Ordered, unique index on file descriptors to make scheduler main loop's
    // lookups fast.
    boost::multi_index::ordered_unique<
      boost::multi_index::tag<fd_tag>,
      boost::multi_index::member<
        io_callback_entry,
        int,
        &io_callback_entry::m_fd
      >
    >,

    // Sequenced index for finding matches for event masks; used during
    // registration/deregistration.
    // XXX: a better index would be possible, but boost does not seem to support
    // it.
    boost::multi_index::sequenced<
      boost::multi_index::tag<events_tag>
    >
  >
> io_callbacks_t;


// 2. Scheduled callbacks:
//
//  - The ideal for scheduling is to find all callbacks whose scheduled time
//    is equal to or exceeds now().
//    That means, the ideal is for the next scheduled time to the key to a
//    sorted container.
//  - The key needs to be non-unique: multiple callbacks can occur at the
//    same time. Similarly, the value needs to be non-unique: the same
//    callback can be scheduled at multiple times.
//  - The value type to the above map would be (callback, metadata), where
//    the metadata describes e.g. the scheduling interval, etc.
//  - Since callbacks can be scheduled at intervals, it is imperative that
//    the key can be modified, causing a re-sort of the container.

// Tags
struct timeout_tag {};
struct callback_tag {};

struct scheduled_callback_entry : public callback_entry
{
  // Invocation time for the callback.
  duration::usec_t      m_timeout;
  // Zero if callback is one-shot.
  // Negative if callback is to be repeated until cancelled.
  // A positive number gives the number of repeats.
  ssize_t               m_count;
  // If non-zero, re-schedule the callback
  duration::usec_t      m_interval;


  scheduled_callback_entry(callback cb, duration::usec_t const & timeout,
      ssize_t count, duration::usec_t const & interval)
    : callback_entry(CB_ENTRY_SCHEDULED, cb)
    , m_timeout(timeout)
    , m_count(count)
    , m_interval(interval)
  {
  }

  scheduled_callback_entry()
    : callback_entry(CB_ENTRY_SCHEDULED)
    , m_timeout()
    , m_count()
    , m_interval()
  {
  }
};

typedef boost::multi_index_container<
  scheduled_callback_entry *,

  boost::multi_index::indexed_by<
    // Ordered, non-unique index on timeouts to make scheduler main loop's
    // lookups fast.
    boost::multi_index::ordered_non_unique<
      boost::multi_index::tag<timeout_tag>,
      boost::multi_index::member<
        scheduled_callback_entry,
        duration::usec_t,
        &scheduled_callback_entry::m_timeout
      >
    >,

    // Hashed, non-unique index for finding callbacks quickly.
    boost::multi_index::hashed_non_unique<
      boost::multi_index::tag<callback_tag>,
      boost::multi_index::member<
        callback_entry,
        callback,
        &callback_entry::m_callback
      >
    >
  >
> scheduled_callbacks_t;


// 3. User-defined callbacks:
//
//  - There are no file descriptors involved. We just map from events to
//    callbacks (and back for unregistering, etc.)
//  - The lookup occurs via the event mask.
//  - Lookup happens both within the scheduler (system events) and on the
//    caller side (via fire_events()), so should be fast. Since the event
//    mask is a bitset, it's likely that an ordered key set will yield the
//    fastest lookup.
//  - The value set is non-unique: the same event mask can be associated with
//    multiple callbacks.
//    With traditional containers (e.g. map<events, list<callback>>) that
//    would provide for tricky merging problems when the event mask for one
//    of the callbacks is modified, but the rest remains unaffected.
//  - The key needs to be mutable (see above).

struct user_callback_entry : public callback_entry
{
  uint64_t      m_events;

  user_callback_entry(callback cb, uint64_t const & events)
    : callback_entry(CB_ENTRY_USER, cb)
    , m_events(events)
  {
  }

  user_callback_entry(uint64_t const & events)
    : callback_entry(CB_ENTRY_USER)
    , m_events(events)
  {
  }

  user_callback_entry()
    : callback_entry(CB_ENTRY_USER)
    , m_events()
  {
  }
};

typedef boost::multi_index_container<
  user_callback_entry *,

  boost::multi_index::indexed_by<
    // Sequenced index for finding matches for event masks; used during
    // registration/deregistration.
    // XXX: a better index would be possible, but boost does not seem to support
    // it.
    boost::multi_index::sequenced<
      boost::multi_index::tag<events_tag>
    >,

    // Hashed, non-unique index for finding callbacks quickly.
    boost::multi_index::hashed_non_unique<
      boost::multi_index::tag<callback_tag>,
      boost::multi_index::member<
        callback_entry,
        callback,
        &callback_entry::m_callback
      >
    >
  >
> user_callbacks_t;

} // namespace detail

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
    uint64_t  events;
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
  void main_scheduler_loop();

  inline void process_in_queue(entry_list_t & triggered);
  inline void process_in_queue_io(action_type action,
      detail::io_callback_entry * entry);
  inline void process_in_queue_scheduled(action_type action,
      detail::scheduled_callback_entry * entry);
  inline void process_in_queue_user(action_type action,
      detail::user_callback_entry * entry, entry_list_t & triggered);

  inline void dispatch_scheduled_callbacks(duration::usec_t const & now,
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
  pipe                            m_worker_pipe;

  // Main loop state.
  std::atomic<bool>               m_main_loop_continue;
  boost::thread                   m_main_loop_thread;
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
#if defined(HAVE_SYS_EPOLL_H)
  int       m_epoll_fd;
#endif

};

} // namespace packetflinger

#endif // guard
