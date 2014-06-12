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

// Scheduled callbacks:
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



struct scheduled_callbacks_t
{
public:
  typedef std::vector<scheduled_callback_entry *> list_t;

  scheduled_callbacks_t()
  {
  }



  inline void add(scheduled_callback_entry * entry)
  {
    // No magic. If the same callback gets added for the same timeout, it
    // deliberately gets called multiple times.
    m_timeout_map.insert(timeout_map_t::value_type(entry->m_timeout, entry));
  }



  inline void remove(scheduled_callback_entry * entry)
  {
    remove_internal(entry, true);
  }



  list_t
  get_timed_out(twine::chrono::nanoseconds const & now) const
  {
    auto end = m_timeout_map.upper_bound(now);
    list_t ret;
    for (auto iter = m_timeout_map.begin() ; iter != end ; ++iter) {
      ret.push_back(iter->second);
    }
    return ret;
  }



  void update(list_t const & erase, list_t const & reschedule)
  {
    // Erase the erase list
    for (auto entry : erase) {
      remove_internal(entry, true);
    }

    // Erase, but don't destroy the reschedule list. Re-add it again, as the
    // timeout has changed.
    for (auto entry : reschedule) {
      remove_internal(entry, false);
      entry->m_timeout += entry->m_interval;
      add(entry);
    }
  }

private:
  inline void remove_internal(scheduled_callback_entry * entry, bool destroy)
  {
    // This is trickier. There are likely multiple entries that match the timeout,
    // and some that also match the callback. We need to remove the latter, those
    // that match both.
    auto range = m_timeout_map.equal_range(entry->m_timeout);

    std::vector<timeout_map_t::const_iterator> to_erase;
    for (auto iter = range.first ; iter != range.second ; ++iter) {
      if (entry->m_callback == iter->second->m_callback) {
        if (destroy) {
          delete iter->second;
        }
        to_erase.push_back(iter);
      }
    }


    for (auto iter : to_erase) {
      m_timeout_map.erase(iter);
    }
  }


  typedef std::multimap<
    twine::chrono::nanoseconds,
    scheduled_callback_entry *
  > timeout_map_t;

  timeout_map_t   m_timeout_map;
};



}} // namespace packetflinger::detail

#endif // guard
