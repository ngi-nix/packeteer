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
    LOG("Trying to add callback for events " << cb->m_events);
    // Try to find the pointer first. If it's in the pointer set, it's also in
    // the events map.
    LOG("callback hashes to: " << cb->m_callback.hash());
    LOG("map type: " << typeid(m_callback_map).name());
    auto c_iter = m_callback_map.find(cb->m_callback);
    if (m_callback_map.end() == c_iter) {
      LOG("New entry!");
      // New entry!
      m_callback_map[cb->m_callback] = cb;
      m_events_map[cb->m_events] = cb;
    }
    else {
      // Existing entry. We don't modify the pointer set, but we do need to
      // remove and re-add the cb into the events map, to keep the mapping
      // up-to-date.
      LOG("Existing entry had " << c_iter->second->m_events);
      remove_from_events(c_iter->second);
      c_iter->second->m_events |= cb->m_events;
      LOG("Now has " << c_iter->second->m_events);
      m_events_map[c_iter->second->m_events] = c_iter->second;
    }
  }



  inline void remove(user_callback_entry * cb)
  {
    // Try to find the pointer first. If it's in the pointer set, it's also in
    // the events map.
    LOG("trying to remove entry with mask " << cb->m_events);
    auto c_iter = m_callback_map.find(cb->m_callback);
    if (m_callback_map.end() == c_iter) {
      // Not found, just stop doing stuff.
      LOG("did not find the callback, skipping.");
      return;
    }

    // We know we need to remove the entry from the events set again, because
    // the event mask will change.
    remove_from_events(c_iter->second);
    c_iter->second->m_events &= ~(cb->m_events);

    // Only re-add the entry if there's an event left.
    if (c_iter->second->m_events) {
      m_events_map[c_iter->second->m_events] = c_iter->second;
    }
    else {
      m_callback_map.erase(c_iter);
      delete c_iter->second;
    }
  }


  std::vector<user_callback_entry *>
  copy_matching(events_t const & events) const
  {
    std::vector<user_callback_entry *> result;

    // Ignore events that don't match at all.
    LOG("looking for events: " << events);
// FIXME there must be an optimization here
//    auto begin = m_events_map.lower_bound(events);
//    auto end = m_events_map.end();
    auto begin = m_events_map.begin();
    auto end = m_events_map.end();

    for (auto iter = begin ; iter != end ; ++iter) {
      LOG("got candiate with: " << iter->first);
      events_t masked = iter->first & events;
      if (masked) {
        auto copy = new user_callback_entry(*(iter)->second);
        copy->m_events = masked;
        result.push_back(copy);
      }
    }

    return result;
  }

private:
  // The fastest way to find a callback is by a hash over the pointer.
  packetflinger_hash_map<callback, user_callback_entry *> m_callback_map;

  // The fastest way to find a callback by event is to have the callbacks sorted
  // by the event integer; that way, we'll quickly filter out events that don't
  // match the mask at all.
  std::map<events_t, user_callback_entry *> m_events_map;

  inline void remove_from_events(user_callback_entry * cb)
  {
    auto range = m_events_map.equal_range(cb->m_events);
    for (auto iter = range.first ; iter != range.second ; ++iter) {
      if (cb == iter->second) {
        m_events_map.erase(iter);
        break;
      }
    }
  }
};


}} // namespace packetflinger::detail

#endif // guardf
