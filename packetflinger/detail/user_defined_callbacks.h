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
#ifndef PACKETFLINGER_DETAIL_USER_DEFINED_CALLBACKS_H
#define PACKETFLINGER_DETAIL_USER_DEFINED_CALLBACKS_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packetflinger/packetflinger.h>

#include <packetflinger/types.h>

#include <meta/range.h>


namespace packetflinger {
namespace detail {

// User-defined callbacks:
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
  events_t      m_events;

  user_callback_entry(callback cb, events_t const & events)
    : callback_entry(CB_ENTRY_USER, cb)
    , m_events(events)
  {
  }

  user_callback_entry(events_t const & events)
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


// Adding or removing events means one of two things:
// - If the callback is already known as a callback for user events, the new
//   event mask will be added to/subtracted from the existing one. If due to
//   subtraction an event mask reaches zero, the entry is removed entirely.
// - In the case of addtion, if the callback is not yet known, the entry will
//   be added verbatim.
struct user_callbacks_t
{
public:
  user_callbacks_t()
  {
  }



  inline void add(user_callback_entry * cb)
  {
    // Try to find the callback first.
    auto c_iter = m_callback_map.find(cb->m_callback);
    if (m_callback_map.end() == c_iter) {
      // New entry!
      m_callback_map[cb->m_callback] = cb;
    }
    else {
      // Existing entry, merge mask
      c_iter->second->m_events |= cb->m_events;
    }
  }



  inline void remove(user_callback_entry * cb)
  {
    // Try to find the callback first.
    auto c_iter = m_callback_map.find(cb->m_callback);
    if (m_callback_map.end() == c_iter) {
      // Not found, just stop doing stuff.
      return;
    }

    // Remove the masked bits
    c_iter->second->m_events &= ~(cb->m_events);

    // Erase/delete the entry if there's no mask left.
    if (!c_iter->second->m_events) {
      delete c_iter->second;
      m_callback_map.erase(c_iter);
    }
  }


  std::vector<user_callback_entry *>
  copy_matching(events_t const & events) const
  {
    std::vector<user_callback_entry *> result;

    // LOG("Looking for events matching: " << events);

    // Iterate over all entries. There's no (easy) optimization for matching
    // bitmasks here.
    auto end = m_callback_map.end();

    for (auto iter = m_callback_map.begin() ; iter != end ; ++iter) {
      events_t masked = iter->second->m_events & events;
      // LOG("mask of " << iter->second->m_events << " is " << masked);
      if (masked) {
        auto copy = new user_callback_entry(*(iter)->second);
        copy->m_events = masked;
        result.push_back(copy);
      }
    }

    return result;
  }

private:
  // The fastest way to find a callback is by a hash.
  packetflinger_hash_map<callback, user_callback_entry *> m_callback_map;
};


}} // namespace packetflinger::detail

#endif // guardf
