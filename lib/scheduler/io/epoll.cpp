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
#include <build-config.h>

#include "epoll.h"

#include "../../globals.h"
#include "../scheduler_impl.h"

#include <unistd.h>
#include <sys/epoll.h>
#include <errno.h>

#include <cstring>
#include <chrono>

#include <packeteer/error.h>
#include <packeteer/types.h>
#include <packeteer/connector.h>

namespace sc = std::chrono;

namespace packeteer::detail {

namespace {

inline int
translate_events_to_os(events_t const & events)
{
  int ret = 0;

  if (events & PEV_IO_READ) {
    ret |= EPOLLIN | EPOLLPRI;
  }
  if (events & PEV_IO_WRITE) {
    ret |= EPOLLOUT;
  }
  if (events & PEV_IO_CLOSE) {
    ret |= EPOLLRDHUP | EPOLLHUP;
  }
  if (events & PEV_IO_ERROR) {
    ret |= EPOLLERR;
  }

  return ret;
}



inline events_t
translate_os_to_events(int os)
{
  events_t ret = 0;

  if ((os & EPOLLIN) || (os & EPOLLPRI)) {
    ret |= PEV_IO_READ;
  }
  if (os & EPOLLOUT) {
    ret |= PEV_IO_WRITE;
  }
  if ((os & EPOLLRDHUP) || (os & EPOLLHUP)) {
    ret |= PEV_IO_CLOSE;
  }
  if (os & EPOLLERR) {
    ret |= PEV_IO_ERROR;
  }

  return ret;
}



inline void
modify_fd_set(int epoll_fd, int action, int const * fds, size_t size,
    events_t const & events)
  OCLINT_SUPPRESS("high cyclomatic complexity")
{
  int translated = translate_events_to_os(events);

  for (size_t i = 0 ; i < size ; ++i) {
    ::epoll_event event;
    ::memset(&event, 0, sizeof(event));
    event.events = translated;
    event.data.fd = fds[i];
    int ret = ::epoll_ctl(epoll_fd, action, fds[i], &event);

    if (ret >= 0) {
      continue;
    }

    // Error handling
    switch (errno) {
      case EEXIST:
        if (EPOLL_CTL_ADD == action) {
          int again_fds[] = { fds[i] };
          modify_fd_set(epoll_fd, EPOLL_CTL_MOD, again_fds,
              sizeof(again_fds) / sizeof(int), events);
        }
        else {
          throw exception(ERR_UNEXPECTED, errno);
        }
        break;

      case ENOENT:
        if (EPOLL_CTL_DEL == action) { //!OCLINT
          // silently ignore
        }
        else if (EPOLL_CTL_MOD == action) {
          // Can only be reached via the modify_fd_set call above, so we are
          // safe to just bail out.
          throw exception(ERR_INVALID_VALUE, errno, "Cannot modify event mask "
              "for unknown file descriptor.");
        }
        else {
          throw exception(ERR_UNEXPECTED, errno);
        }
        break;

      case ENOMEM:
        throw exception(ERR_OUT_OF_MEMORY, errno, "No more memory for epoll.");
        break;

      case ENOSPC:
        throw exception(ERR_NUM_FILES, errno, "Could not register new file "
            "descriptor.");
        break;

      case EBADF:
      case EINVAL:
      case EPERM:
        throw exception(ERR_INVALID_VALUE, errno, "Invalid file descriptor "
            "provided.");
        break;

      default:
        throw exception(ERR_UNEXPECTED, errno);
        break;
    }
  }
}


inline void
modify_conn_set(int epoll_fd, int action, connector const * conns, size_t size,
    events_t const & events)
{
  // We treat error, etc. events as applying to both the read and write end of
  // each connector, but apply PEV_IO_READ only to the read end, and
  // PEV_IO_WRITE only to the write end. If both handles are the same, we let
  // the system figure that out.
  std::vector<int> readers;
  readers.reserve(size);
  std::vector<int> writers;
  writers.reserve(size);

  for (size_t i = 0 ; i < size ; ++i) {
    connector const & conn = conns[i];
    if (events & PEV_IO_READ) {
      readers.push_back(conn.get_read_handle().sys_handle());
    }
    if (events & PEV_IO_WRITE) {
      writers.push_back(conn.get_write_handle().sys_handle());
    }
  }

  modify_fd_set(epoll_fd, action, &readers[0], readers.size(),
      events | ~PEV_IO_WRITE);
  modify_fd_set(epoll_fd, action, &writers[0], writers.size(),
      events | ~PEV_IO_READ);
}


} // anonymous namespace



io_epoll::io_epoll(std::shared_ptr<api> api)
  : io(api)
  , m_epoll_fd(-1)
{
  int res = ::epoll_create1(EPOLL_CLOEXEC);
  if (res < 0) {
    switch (errno) {
      case EMFILE:
      case ENFILE:
        throw exception(ERR_NUM_FILES, errno, "Could not create epoll file "
            "descriptor.");
        break;

      case ENOMEM:
        throw exception(ERR_OUT_OF_MEMORY, errno, "Could not create epoll file "
            "descriptor.");
        break;

      default:
        throw exception(ERR_UNEXPECTED, errno);
        break;
    }
  }
  else {
    m_epoll_fd = res;
  }

  DLOG("Epoll based I/O subsystem created.");
}



io_epoll::~io_epoll()
{
  if (-1 != m_epoll_fd) {
    ::close(m_epoll_fd);
    m_epoll_fd = -1;
  }
}




void
io_epoll::register_connector(connector const & conn, events_t const & events)
{
  connector conns[] = { conn };
  constexpr auto size = sizeof(conns) / sizeof(connector);

  io::register_connectors(conns, size, events);

  modify_conn_set(m_epoll_fd, EPOLL_CTL_ADD, conns, size, events);
}



void
io_epoll::register_connectors(connector const * conns, size_t size,
    events_t const & events)
{
  io::register_connectors(conns, size, events);

  modify_conn_set(m_epoll_fd, EPOLL_CTL_ADD, conns, size, events);
}




void
io_epoll::unregister_connector(connector const & conn, events_t const & events)
{
  connector conns[] = { conn };
  constexpr auto size = sizeof(conns) / sizeof(connector);

  io::unregister_connectors(conns, size, events);

  modify_conn_set(m_epoll_fd, EPOLL_CTL_DEL, conns, size, events);
}



void
io_epoll::unregister_connectors(connector const * conns, size_t size,
    events_t const & events)
{
  io::unregister_connectors(conns, size, events);

  modify_conn_set(m_epoll_fd, EPOLL_CTL_DEL, conns, size, events);
}



void
io_epoll::wait_for_events(std::vector<event_data> & events,
      duration const & timeout)
{
  // Wait for events
  ::epoll_event epoll_events[PACKETEER_EPOLL_MAXEVENTS];
  int ready = -1;

  while (true) {
    ::memset(&epoll_events, 0, sizeof(epoll_events));
    ready = ::epoll_pwait(m_epoll_fd, epoll_events, PACKETEER_EPOLL_MAXEVENTS,
        sc::ceil<sc::milliseconds>(timeout).count(), nullptr);
    if (-1 != ready) {
      break;
    }

    // Error handling
    switch (errno) {
      case EINTR: // signal interrupt handling
        continue;

      case EBADF:
      case EINVAL:
        throw exception(ERR_INVALID_VALUE, errno, "File descriptor for epoll "
            "was invalid.");
        break;

      default:
        throw exception(ERR_UNEXPECTED, errno);
        break;
    }
  }

  // Translate events
  for (int i = 0 ; i < ready ; ++i) {
    event_data data = {
      m_connectors[epoll_events[i].data.fd],
      translate_os_to_events(epoll_events[i].events)
    };
    events.push_back(data);
  }
}



} // namespace packeteer::detail
