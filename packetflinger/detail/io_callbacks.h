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
#ifndef PACKETFLINGER_DETAIL_IO_CALLBACKS_H
#define PACKETFLINGER_DETAIL_IO_CALLBACKS_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packetflinger/packetflinger.h>

#include <packetflinger/types.h>

namespace packetflinger {
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

  io_callback_entry()
    : callback_entry(CB_ENTRY_IO)
    , m_fd()
    , m_events()
  {
  }
};

/* FIXME
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
*/

struct io_callbacks_t
{
  io_callbacks_t()
  {
  }

  // FIXME
  // packetflinger_hash_map<int, io_callback_entry> m_map;
};



}} // namespace packetflinger::detail

#endif // guard
