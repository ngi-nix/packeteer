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
#include <packeteer/detail/io_epoll.h>
#include <packeteer/detail/scheduler_impl.h>

#include <sys/epoll.h>
#include <errno.h>

#include <cstring>

#include <packeteer/error.h>
#include <packeteer/types.h>
#include <packeteer/events.h>

#define MAXEVENTS 64 // FIXME good number?

namespace packeteer {
namespace detail {

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
{
  int translated = translate_events_to_os(events);

  for (size_t i = 0 ; i < size ; ++i) {
    ::epoll_event event;
    ::memset(&event, 0, sizeof(event));
    event.events = translated;
    event.data.fd = fds[i];
    int ret = ::epoll_ctl(epoll_fd, action, fds[i], &event);

    if (-1 == ret) {
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
          if (EPOLL_CTL_DEL == action) {
            // silently ignore
          }
          else if (EPOLL_CTL_MOD == action) {
            // Can only be reached via the modify_fd_set call above, so we are
            // safe to just bail out.
            throw exception(ERR_INVALID_VALUE, "Cannot modify event mask for unknown file descriptor.");
          }
          else {
            throw exception(ERR_UNEXPECTED, errno);
          }
          break;

        case ENOMEM:
          throw exception(ERR_OUT_OF_MEMORY, "No more memory for epoll.");
          break;

        case ENOSPC:
          throw exception(ERR_NUM_FILES, "Could not register new file descriptor.");
          break;

        case EBADF:
        case EINVAL:
        case EPERM:
          throw exception(ERR_INVALID_VALUE, "Invalid file descriptor provided.");
          break;

        default:
          throw exception(ERR_UNEXPECTED, errno);
          break;
      }
    }
  }
}


} // anonymous namespace



io_epoll::io_epoll()
  : m_epoll_fd(-1)
{
  int res = ::epoll_create1(EPOLL_CLOEXEC);
  if (res < 0) {
    switch (errno) {
      case EMFILE:
      case ENFILE:
        throw exception(ERR_NUM_FILES, "Could not create epoll file descriptor.");
        break;

      case ENOMEM:
        throw exception(ERR_OUT_OF_MEMORY, "Could not create epoll file descriptor.");
        break;

      default:
        throw exception(ERR_UNEXPECTED, errno);
        break;
    }
  }
  else {
    m_epoll_fd = res;
  }

  LOG("Epoll based I/O subsystem created.");
}



io_epoll::~io_epoll()
{
  if (-1 != m_epoll_fd) {
    ::close(m_epoll_fd);
    m_epoll_fd = -1;
  }
}




void
io_epoll::register_fd(int fd, events_t const & events)
{
  int fds[] = { fd };
  modify_fd_set(m_epoll_fd, EPOLL_CTL_ADD, fds, sizeof(fds) / sizeof(int),
      events);
}



void
io_epoll::register_fds(int const * fds, size_t size,
    events_t const & events)
{
  modify_fd_set(m_epoll_fd, EPOLL_CTL_ADD, fds, size, events);
}




void
io_epoll::unregister_fd(int fd, events_t const & events)
{
  int fds[] = { fd };
  modify_fd_set(m_epoll_fd, EPOLL_CTL_DEL, fds, sizeof(fds) / sizeof(int),
      events);
}



void
io_epoll::unregister_fds(int const * fds, size_t size,
    events_t const & events)
{
  modify_fd_set(m_epoll_fd, EPOLL_CTL_DEL, fds, size, events);
}



void
io_epoll::wait_for_events(std::vector<event_data> & events,
      twine::chrono::nanoseconds const & timeout)
{
  // FIXME also set signal mask & handle it accordingly

  // Wait for events
  ::epoll_event epoll_events[MAXEVENTS];
  ::memset(&epoll_events, 0, sizeof(epoll_events));
  int ready = ::epoll_pwait(m_epoll_fd, epoll_events, MAXEVENTS,
      timeout.as<twine::chrono::milliseconds>(), nullptr);

  // Error handling
  if (-1 == ready) {
    switch (errno) {
      case EBADF:
      case EINVAL:
        throw exception(ERR_INVALID_VALUE, "File descriptor for epoll was invalid.");
        break;

      case EINTR:
        // FIXME add signal handling

      default:
        throw exception(ERR_UNEXPECTED, errno);
        break;
    }
  }

  // Translate events
  for (int i = 0 ; i < ready ; ++i) {
    event_data data = {
      epoll_events[i].data.fd,
      translate_os_to_events(epoll_events[i].events)
    };
    events.push_back(data);
  }
}



}} // namespace packeteer::detail
