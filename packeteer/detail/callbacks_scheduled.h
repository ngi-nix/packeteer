/**
 * This file is part of packeteer.
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
#ifndef PACKETEER_DETAIL_CALLBACKS_SCHEDULED_H
#define PACKETEER_DETAIL_CALLBACKS_SCHEDULED_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer/packeteer.h>

#include <packeteer/types.h>

#include <map>

namespace packeteer {
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


  scheduled_callback_entry(callback const & cb,
      twine::chrono::nanoseconds const & timeout, ssize_t count = 0,
      twine::chrono::nanoseconds const & interval = twine::chrono::nanoseconds(0))
    : callback_entry(CB_ENTRY_SCHEDULED, cb)
    , m_timeout(timeout)
    , m_count(count)
    , m_interval(interval)
  {
  }

  // Automatic copy constructor is used
};



struct scheduled_callbacks_t
{
public:
  typedef std::vector<scheduled_callback_entry *> list_t;

  scheduled_callbacks_t()
  {
  }


  ~scheduled_callbacks_t()
  {
    for (auto entry : m_timeout_map) {
      delete entry.second;
    }
  }


  /**
   * Takes ownership of the passed entry.
   *
   * Adds the entry to the container. Note that multiple timeouts for the same
   * callback are supported, as are multiple callbacks for the same timeout.
   **/
  inline void add(scheduled_callback_entry * entry)
  {
    // No magic. If the same callback gets added for the same timeout, it
    // deliberately gets called multiple times.
    m_timeout_map.insert(timeout_map_t::value_type(entry->m_timeout, entry));
  }



  /**
   * Removes and deletes any entry from the container that matches the passed
   * entry's callback *ONLY*.
   **/
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



  /**
   * Updates the container:
   * - The first parameter contains entries that have been taken out of the
   *   container; that is, their ownership has passed elsewhere. We remove them,
   *   but don't destroy them.
   * - The second parameter contains entries who need to be run again later;
   *   for these, we need to update the timeout (via the interval). Ownership
   *   remains with the container here!
   **/
  void update(list_t const & erase, list_t const & reschedule)
  {
    // Erase the erase list
    for (auto entry : erase) {
      remove_internal(entry, false);
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
    auto end = m_timeout_map.end();

    // LOG("trying to remove entry with timeout: " << entry->m_timeout);

    for (auto iter = m_timeout_map.begin() ; iter != end ; ) {
      // LOG("got " << iter->second->m_timeout);
      if (entry->m_callback == iter->second->m_callback) {
        if (destroy) {
          delete iter->second;
        }
        m_timeout_map.erase(iter++);
      }
      else {
        ++iter;
      }
    }
  }


  typedef std::multimap<
    twine::chrono::nanoseconds,
    scheduled_callback_entry *
  > timeout_map_t;

  timeout_map_t   m_timeout_map;
};



}} // namespace packeteer::detail

#endif // guard
