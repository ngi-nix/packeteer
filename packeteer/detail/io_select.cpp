/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 * Copyright (c) 2012-2014 Unwesen Ltd.
 * Copyright (c) 2015-2019 Jens Finkhaeuser.
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
#include <packeteer/detail/io_select.h>
#include <packeteer/detail/scheduler_impl.h>

// Posix
#include <sys/select.h>

// Earlier standards
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <errno.h>

#include <packeteer/error.h>
#include <packeteer/types.h>
#include <packeteer/events.h>


namespace packeteer {
namespace detail {


io_select::io_select()
{
  LOG("Select based I/O subsystem created.");
}



io_select::~io_select()
{
}



void
io_select::register_handle(handle const & h, events_t const & events)
{
  m_fds[h.sys_handle()] |= events;
}



void
io_select::register_handles(handle const * handles, size_t size,
    events_t const & events)
{
  for (size_t i = 0 ; i < size ; ++i) {
    m_fds[handles[i].sys_handle()] |= events;
  }
}




void
io_select::unregister_handle(handle const & h, events_t const & events)
{
  auto iter = m_fds.find(h.sys_handle());
  iter->second &= ~events;
  if (!iter->second) {
    m_fds.erase(iter);
  }
}



void
io_select::unregister_handles(handle const * handles, size_t size,
    events_t const & events)
{
  for (size_t i = 0 ; i < size ; ++i) {
    auto iter = m_fds.find(handles[i].sys_handle());
    iter->second &= ~events;
    if (!iter->second) {
      m_fds.erase(iter);
    }
  }
}



void
io_select::wait_for_events(std::vector<event_data> & events,
      twine::chrono::nanoseconds const & timeout)
{
  // FD sets
  ::fd_set read_fds;
  ::fd_set write_fds;
  ::fd_set err_fds;

  while (true) {
    // Prepare FD sets.
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&err_fds);

    // Populate FD sets.
    int max_fd = 0;
    for (auto entry : m_fds) {
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
    timeout.as(ts);

    int ret = ::pselect(max_fd + 1, &read_fds, &write_fds, &err_fds, &ts, nullptr);
#else
    ::timeval tv;
    timeout.as(tv);

    int ret = ::select(max_fd + 1, &read_fds, &write_fds, &err_fds, &tv);
#endif

    if (ret >= 0) {
      break;
    }

    // Error handling
    switch (errno) {
      case EINTR: // signal interrupt handling
        continue;

      case EBADF:
      case EINVAL:
        throw exception(ERR_INVALID_VALUE, errno, "Bad file descriptor in select set.");

      case ENOMEM:
        throw exception(ERR_OUT_OF_MEMORY, errno, "OOM in select call.");

      default:
        throw exception(ERR_UNEXPECTED, errno);
    }
  }

  // Map events; we'll need to iterate over the available file descriptors again
  // (conceivably, we could just use the subset in the FD sets, but that uses
  // additional memory).
  for (auto entry : m_fds) {
    int mask = 0;
    if (FD_ISSET(entry.first, &read_fds)) {
      mask |= PEV_IO_READ;
    }
    if (FD_ISSET(entry.first, &write_fds)) {
      mask |= PEV_IO_WRITE;
    }
    if (FD_ISSET(entry.first, &err_fds)) {
      mask |= PEV_IO_ERROR;
    }

    if (mask) {
      event_data ev;
      ev.m_handle = handle(entry.first);
      ev.m_events = mask;
      events.push_back(ev);
    }
  }
}



}} // namespace packeteer::detail
