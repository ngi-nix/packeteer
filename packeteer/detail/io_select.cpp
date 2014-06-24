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



void
io_select::init()
{
}



void
io_select::deinit()
{
}



void
io_select::register_fd(int fd, events_t const & events)
{
  m_fds[fd] |= events;
}



void
io_select::register_fds(int const * fds, size_t size,
    events_t const & events)
{
  for (int i = 0 ; i < size ; ++i) {
    m_fds[fds[i]] |= events;
  }
}




void
io_select::unregister_fd(int fd, events_t const & events)
{
  auto iter = m_fds.find(fd);
  iter->second &= ~events;
  if (!iter->second) {
    m_fds.erase(iter);
  }
}



void
io_select::unregister_fds(int const * fds, size_t size,
    events_t const & events)
{
  for (int i = 0 ; i < size ; ++i) {
    auto iter = m_fds.find(fds[i]);
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
  while (true) {
    // FIXME also set signal mask & handle it accordingly

    // Prepare FD sets.
    ::fd_set read_fds; FD_ZERO(&read_fds);
    ::fd_set write_fds; FD_ZERO(&write_fds);
    ::fd_set err_fds; FD_ZERO(&err_fds);

    // Populate FD sets.
    int max_fd = 0;
    for (auto entry : m_fds) {
      if (entry.first > max_fd) {
        max_fd = entry.first;
      }

      if (entry.second & EV_IO_READ) {
        FD_SET(entry.first, &read_fds);
      }
      if (entry.second & EV_IO_WRITE) {
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

    // Error handling
    if (ret < 0) {
      switch (errno) {
        case EBADF:
        case EINVAL:
          throw exception(ERR_INVALID_VALUE, "Bad file descriptor in select set.");
          break;

        case EINTR:
          // FIXME add proper signal handling; until then, try again.
          continue;

        case ENOMEM:
          throw exception(ERR_OUT_OF_MEMORY, "OOM in select call.");
          break;

        default:
          throw exception(ERR_UNEXPECTED, errno);
          break;
      }
    }

    // Map events; we'll need to iterate over the available file descriptors again
    // (conceivably, we could just use the subset in the FD sets, but that uses
    // additional memory).
    for (auto entry : m_fds) {
      int mask = 0;
      if (FD_ISSET(entry.first, &read_fds)) {
        mask |= EV_IO_READ;
      }
      if (FD_ISSET(entry.first, &write_fds)) {
        mask |= EV_IO_WRITE;
      }
      if (FD_ISSET(entry.first, &err_fds)) {
        mask |= EV_IO_ERROR;
      }

      if (mask) {
        event_data ev;
        ev.fd = entry.first;
        ev.events = mask;
        events.push_back(ev);
      }
    }

    // Break out of the loop. We only continue in the EINTR case above.
    break;
  }
}



}} // namespace packeteer::detail
