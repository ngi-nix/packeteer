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
#ifndef PACKETFLINGER_DETAIL_SCHEDULED_CALLBACKS_H
#define PACKETFLINGER_DETAIL_SCHEDULED_CALLBACKS_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packetflinger/packetflinger.h>

#include <packetflinger/types.h>

#include <meta/range.h>

namespace packetflinger {
namespace detail {

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

struct scheduled_callback_entry : public callback_entry
{
  // Invocation time for the callback.
  twine::chrono::nanoseconds  m_timeout;
  // Zero if callback is one-shot.
  // Negative if callback is to be repeated until cancelled.
  // A positive number gives the number of repeats.
  ssize_t                     m_count;
  // If non-zero, re-schedule the callback
  twine::chrono::nanoseconds  m_interval;


  scheduled_callback_entry(callback cb,
      twine::chrono::nanoseconds const & timeout, ssize_t count,
      twine::chrono::nanoseconds const & interval)
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

/* FIXME
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
*/

struct scheduled_callbacks_t
{
  scheduled_callbacks_t();

  // FIXME packetflinger_hash_map  m_map;
  //
  void insert(scheduled_callback_entry *);

  void erase(callback const & cb);

  // FIXME
  typedef std::vector<scheduled_callback_entry *> foo;

  meta::range<foo::iterator>
  get_timed_out(twine::chrono::nanoseconds const & now);

  void erase_timed_out(twine::chrono::nanoseconds const & now);
};



}} // namespace packetflinger::detail

#endif // guard
