/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 * Copyright (c) 2012-2014 Unwesen Ltd.
 * Copyright (c) 2015-2020 Jens Finkhaeuser.
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

#include <packeteer/error.h>

#include "../../scheduler_impl.h"
#include "../../../chrono.h"

// Posix
#include <sys/select.h>

// Earlier standards
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <errno.h>

#include <chrono>

namespace sc = std::chrono;

namespace packeteer::detail {

io_select::io_select(std::shared_ptr<api> const & api)
  : io(api)
{
  DLOG("Select based I/O subsystem created.");
}



io_select::~io_select()
{
  DLOG("I/O select subsystem shutting down.");
}



void
io_select::wait_for_events(io_events & events,
      duration const & timeout)
  OCLINT_SUPPRESS("high cyclomatic complexity")
  OCLINT_SUPPRESS("long method")
{
  auto before = clock::now();
  auto cur_timeout = timeout;

  // FD sets
  ::fd_set read_fds;
  ::fd_set write_fds;
  ::fd_set err_fds;

  while (cur_timeout.count() > 0) {
    // Prepare FD sets.
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&err_fds);

    // Populate FD sets.
    int max_fd = 0;
    for (auto entry : m_sys_handles) {
      if (entry.first > max_fd) {
        max_fd = entry.first;
      }

      if (entry.second & PEV_IO_READ) {
        FD_SET(entry.first, &read_fds);
      }
      if (entry.second & PEV_IO_WRITE) {
        FD_SET(entry.first, &write_fds);
      }
      FD_SET(entry.first, &err_fds);
    }

    // Wait for events
#if defined(PACKETEER_HAVE_PSELECT)
    ::timespec ts;
    ::packeteer::thread::chrono::convert(cur_timeout, ts);

    int ret = ::pselect(max_fd + 1, &read_fds, &write_fds, &err_fds, &ts,
        nullptr);
#else
    ::timeval tv;
    ::packeteer::thread::chrono::convert(cur_timeout, tv);

    int ret = ::select(max_fd + 1, &read_fds, &write_fds, &err_fds, &tv);
#endif

    if (ret >= 0) {
      break;
    }

    // Error handling
    switch (errno) {
      case EINTR: // signal interrupt handling
        {
          auto after = clock::now();
          auto tdiff = after - before;
          cur_timeout = timeout - tdiff;
        }
        DLOG("select resuming.");
        continue;

      case EBADF:
      case EINVAL:
        throw exception(ERR_INVALID_VALUE, errno, "Bad file descriptor in "
            "select set.");

      case ENOMEM:
        throw exception(ERR_OUT_OF_MEMORY, errno, "OOM in select call.");

      default:
        throw exception(ERR_UNEXPECTED, errno);
    }
  }

  // Map events; we'll need to iterate over the available file descriptors again
  // (conceivably, we could just use the subset in the FD sets, but that uses
  // additional memory).
  std::map<connector, events_t> tmp_events;
  for (auto entry : m_sys_handles) {
    events_t mask = 0;
    if (FD_ISSET(entry.first, &read_fds)) { //!OCLINT(in FD_ISSET)
      mask |= PEV_IO_READ;
    }
    if (FD_ISSET(entry.first, &write_fds)) { //!OCLINT(in FD_ISSET)
      mask |= PEV_IO_WRITE;
    }
    if (FD_ISSET(entry.first, &err_fds)) { //!OCLINT(in FD_ISSET)
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

  if (!events.empty()) {
    DLOG("select got " << events.size() << " event entries to report.");
  }
}



} // namespace packeteer::detail
