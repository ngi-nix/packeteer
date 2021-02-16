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

#include "epoll.h"

#include "../../../globals.h"
#include "../../scheduler_impl.h"

#include <unistd.h>
#include <sys/epoll.h>
#include <errno.h>

#include <cstring>
#include <chrono>

#include <packeteer/error.h>
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
update_fd_registration_single(int epoll_fd, int action, int fd, events_t events)
{
  ::epoll_event event;
  event.events = translate_events_to_os(events);
  event.data.fd = fd;

  int ret = ::epoll_ctl(epoll_fd, action, fd, &event);
  if (ret >= 0) {
    return;
  }

  // Error handling
  switch (errno) {
    case EEXIST:
      if (EPOLL_CTL_ADD == action) {
        update_fd_registration_single(epoll_fd, EPOLL_CTL_MOD, fd, events);
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
        // Can only be reached via the call above, so we are safe to just bail
        // out.
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


inline void
update_syshandle_registration(int epoll_fd, int fd, io::sys_events_map const & events)
{
  auto iter = events.find(fd);
  if (iter == events.end()) {
    // No events? Need to remove FD entirely.
    update_fd_registration_single(epoll_fd, EPOLL_CTL_DEL, fd, 0);
  }
  else {
    // We have events? Then translate the ones currently registered.
    update_fd_registration_single(epoll_fd, EPOLL_CTL_ADD, fd, iter->second);
  }
}



inline void
update_conn_registration(int epoll_fd, connector const * conns, size_t size,
    io::sys_events_map const & sys_events)
{
  for (size_t i = 0 ; i < size ; ++i) {
    update_syshandle_registration(epoll_fd, conns[i].get_read_handle().sys_handle(),
        sys_events);
    update_syshandle_registration(epoll_fd, conns[i].get_write_handle().sys_handle(),
        sys_events);
  }
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

  update_conn_registration(m_epoll_fd, conns, size, m_sys_handles);
}



void
io_epoll::register_connectors(connector const * conns, size_t size,
    events_t const & events)
{
  io::register_connectors(conns, size, events);

  update_conn_registration(m_epoll_fd, conns, size, m_sys_handles);
}




void
io_epoll::unregister_connector(connector const & conn, events_t const & events)
{
  connector conns[] = { conn };
  constexpr auto size = sizeof(conns) / sizeof(connector);

  io::unregister_connectors(conns, size, events);

  update_conn_registration(m_epoll_fd, conns, size, m_sys_handles);
}



void
io_epoll::unregister_connectors(connector const * conns, size_t size,
    events_t const & events)
{
  io::unregister_connectors(conns, size, events);

  update_conn_registration(m_epoll_fd, conns, size, m_sys_handles);
}



void
io_epoll::wait_for_events(io_events & events,
      duration const & timeout)
{
  auto before = clock::now();
  auto cur_timeout = timeout;

  // Wait for events
  ::epoll_event epoll_events[PACKETEER_EPOLL_MAXEVENTS];
  int ready = -1;

  while (cur_timeout.count() > 0) {
    ready = ::epoll_pwait(m_epoll_fd, epoll_events, PACKETEER_EPOLL_MAXEVENTS,
        sc::ceil<sc::milliseconds>(cur_timeout).count(), nullptr);
    if (-1 != ready) {
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
    io_event data = {
      m_connectors[epoll_events[i].data.fd],
      translate_os_to_events(epoll_events[i].events)
    };
    events.push_back(data);
  }
}



} // namespace packeteer::detail
