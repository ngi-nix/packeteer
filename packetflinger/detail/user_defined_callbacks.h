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

/* FIXME
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
*/

struct user_callbacks_t
{
  typedef user_callback_entry ** iterator; // FIXME
  user_callbacks_t();

  // FIXME packetflinger_hash_map  m_map;
  user_callback_entry * find(callback const & cb);

  void insert(user_callback_entry const * cb);
  void erase(user_callback_entry const * cb);

  meta::range<iterator> get();
};


}} // namespace packetflinger::detail

#endif // guardf
