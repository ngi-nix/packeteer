/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 * Copyright (c) 2012-2014 Unwesen Ltd.
 * Copyright (c) 2015-2017 Jens Finkhaeuser.
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
#ifndef PACKETEER_DETAIL_CALLBACKS_IO_H
#define PACKETEER_DETAIL_CALLBACKS_IO_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer/packeteer.h>

#include <packeteer/types.h>

#include <map>

namespace packeteer {
namespace detail {

// 1. I/O callbacks:
//
//  - While the main scheduler loop will look up metadata with a file
//    descriptor key, the value in this case is a (callback, eventmask)
//    tuple.
//    The above comments on callback uniqueness hold.
//  - We do not care about the ordering of (callback, eventmask).
//  - (callback, eventmask) needs to be modifyable, as users can register
//    and unregister multiple events for the same (callback, FD) tuple.

struct io_callback_entry : public callback_entry
{
  int           m_fd;
  events_t      m_events;

  io_callback_entry(callback const & cb, int fd, events_t const & events)
    : callback_entry(CB_ENTRY_IO, cb)
    , m_fd(fd)
    , m_events(events)
  {
  }

  // Automatic copy constructor is used
};


struct io_callbacks_t
{
  io_callbacks_t()
  {
  }


  ~io_callbacks_t()
  {
    for (auto entry : m_callback_map) {
      delete entry.second;
    }
  }



  /**
   * Takes ownership of the passed entry.
   *
   * If an entry with the same callback/file descriptor exists, their event masks
   * are merged. Otherwise, the entry is added.
   **/
  inline void add(io_callback_entry * cb)
  {
    // Try to find callbacks matching the file descriptor.
    auto range = m_callback_map.equal_range(cb->m_fd);

    // Within these, try to find an entry matching the callback already.
    auto c_iter = range.first;
    for ( ; c_iter != range.second ; ++c_iter) {
      if (cb->m_callback == c_iter->second->m_callback) {
        // Found an entry
        break;
      }
    }

    // Let's see if we found the callback/fd combination
    if (range.second != c_iter) {
      // Yep, found it. Merge event mask.
      c_iter->second->m_events |= cb->m_events;
      delete cb;
    }
    else {
      // Nope, new entry
      m_callback_map.insert(std::make_pair(cb->m_fd, cb));
    }
  }



  /**
   * Removes as much of the passed entry as possible.
   *
   * Primarily, this removes the passed entry's flags from any item in the
   * container matching the callback. If there are no flags left afterwards,
   * the item is removed entirely.
   **/
  inline void remove(io_callback_entry * cb)
  {
    // Try to find callbacks matching the file descriptor
    auto range = m_callback_map.equal_range(cb->m_fd);
    if (range.first == range.second) {
      // Nothing matches this file descriptor
      return;
    }

    // Try to find an entry matching the callback.
    auto c_iter = range.first;
    for ( ; c_iter != range.second ; ++c_iter) {
      if (cb->m_callback == c_iter->second->m_callback) {
        // Found an entry
        break;
      }
    }

    // Let's see if we found the callback/fd combination
    if (range.second != c_iter) {
      // Yep. Remove the event mask bits
      c_iter->second->m_events &= ~(cb->m_events);
      if (!c_iter->second->m_events) {
        delete c_iter->second;
        m_callback_map.erase(c_iter);
      }
    }
    else {
      // Not found, ignoring.
    }
  }



  /**
   * As the name implies, this function creates a copy (ownership goes to the
   * caller) of all entries matching one or more of the events in the passed
   * event mask for the given FD.
   **/
  std::vector<io_callback_entry *>
  copy_matching(int fd, events_t const & events) const
  {
    std::vector<io_callback_entry *> result;

    // Try to find callbacks matching the file descriptor
    auto range = m_callback_map.equal_range(fd);
    if (range.first == range.second) {
      // Nothing matches this file descriptor
      return result;
    }

    // Try to find entries matching the event mask
    auto iter = range.first;
    for ( ; iter != range.second ; ++iter) {
      events_t masked = iter->second->m_events & events;
      if (masked) {
        auto copy = new io_callback_entry(*(iter->second));
        copy->m_events = masked;
        result.push_back(copy);
      }
    }

    return result;
  }


private:
  // For the same file descriptor, we may have multiple callback entries.
  std::multimap<int, io_callback_entry *> m_callback_map;
};



}} // namespace packeteer::detail

#endif // guard
