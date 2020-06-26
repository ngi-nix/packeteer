/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2020 Jens Finkhaeuser.
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
#include <build-config.h>

#include "select.h"

#include "../../../macros.h"
#include "../../../thread/chrono.h"
#include "../../../win32/sys_handle.h"

namespace packeteer::detail {

io_select::io_select(std::shared_ptr<api> const & api)
  : io(api)
{
  DLOG("WIN32 I/O select subsystem created.");
}



io_select::~io_select()
{
  DLOG("WIN32 I/O select subsystem shutting down.");
}



void
io_select::wait_for_events(io_events & events,
      duration const & timeout)
{
  auto before = clock::now();
  auto cur_timeout = timeout;

  // FD sets
  FD_SET read_set;
  FD_SET write_set;
  FD_SET error_set;

  while (cur_timeout.count() > 0) {
    // Prepare FD sets
    FD_ZERO(&read_set);
    FD_ZERO(&write_set);
    FD_ZERO(&error_set);

    // Populate FD sets.
    for (auto entry : m_sys_handles) {
      if (entry.second & PEV_IO_READ) {
        FD_SET(entry.first->socket, &read_set);
      }
      if (entry.second & PEV_IO_WRITE) {
        FD_SET(entry.first->socket, &write_set);
      }
      FD_SET(entry.first->socket, &error_set);
    }

    // Wait for events
    ::timeval tv;
    ::packeteer::thread::chrono::convert(cur_timeout, tv);
    DLOG("WIN32 select for "
        << std::chrono::duration_cast<std::chrono::milliseconds>(cur_timeout).count()
        << "ms.");
    auto total = ::select(0, &read_set, &write_set, &error_set, &tv);

    if (total != SOCKET_ERROR) {
      break;
    }

    // Error handling
    auto err = WSAGetLastError();
    switch (err) {
      case WSANOTINITIALISED:
        throw exception(ERR_INITIALIZATION, err, "WSA not initialized.");

      case WSAEINTR: // Probably similar to posix interrupts
      case WSAEINPROGRESS:
        {
          auto after = clock::now();
          auto tdiff = after - before;
          cur_timeout = timeout - tdiff;
        }
        DLOG("WIN32 select resuming.");
        continue;

      case WSAEFAULT:
        throw exception(ERR_ACCESS_VIOLATION, err);

      case WSAENETDOWN:
        throw exception(ERR_NO_CONNECTION, err);

      case WSAENOTSOCK:
      case WSAEINVAL:
        throw exception(ERR_INVALID_VALUE, err, "Bad file descriptor in "
            "select set.");

      default:
        throw exception(ERR_UNEXPECTED, err);
    }
  }

  // Map events; we'll need to iterate over the available file descriptors again
  // (conceivably, we could just use the subset in the FD sets, but that uses
  // additional memory).
  std::map<connector, events_t> tmp_events;
  for (auto entry : m_sys_handles) {
    events_t mask = 0;
    if (FD_ISSET(entry.first->socket, &read_set)) { //!OCLINT(in FD_ISSET)
      mask |= PEV_IO_READ;
    }
    if (FD_ISSET(entry.first->socket, &write_set)) { //!OCLINT(in FD_ISSET)
      mask |= PEV_IO_WRITE;
    }
    if (FD_ISSET(entry.first->socket, &error_set)) { //!OCLINT(in FD_ISSET)
      mask |= PEV_IO_ERROR;
    }

    if (mask) {
      auto & conn = m_connectors[entry.first];
      if (!conn) {
        ELOG("Got event for unregistered connector with handle: " << handle{entry.first});
        continue;
      }
      tmp_events[conn] |= mask;
    }
  }

  for (auto & [conn, ev] : tmp_events) {
    events.push_back({conn, ev});
  }

  DLOG("WIN32 select got " << events.size() << " event entries to report.");
}

} // namespace packeteer::detail
