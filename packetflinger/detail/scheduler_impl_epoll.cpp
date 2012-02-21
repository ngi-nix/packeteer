/**
 * This file is part of packetflinger.
 *
 * Author(s): Jens Finkhaeuser <jens@unwesen.co.uk>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 * Copyright (c) 2012 Unwesen Ltd.
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
#include <packetflinger/detail/scheduler_impl.h>

#include <sys/epoll.h>
#include <errno.h>

#include <packetflinger/error.h>
#include <packetflinger/types.h>

namespace packetflinger {

namespace {

inline int translate_events_to_os(uint64_t events)
{
  int ret = 0;

  if (events & scheduler::EV_IO_READ) {
    ret |= EPOLLIN | EPOLLPRI;
  }
  if (events & scheduler::EV_IO_WRITE) {
    ret |= EPOLLOUT;
  }
  if (events & scheduler::EV_IO_CLOSE) {
    ret |= EPOLLRDHUP | EPOLLHUP;
  }
  if (events & scheduler::EV_IO_ERROR) {
    ret |= EPOLLERR;
  }

  return ret;
}


inline uint64_t translate_os_to_events(int os)
{
  uint64_t ret = 0;

  if ((os & EPOLLIN) || (os & EPOLLPRI)) {
    ret |= scheduler::EV_IO_READ;
  }
  if (os & EPOLLOUT) {
    ret |= scheduler::EV_IO_WRITE;
  }
  if ((os & EPOLLRDHUP) || (os & EPOLLHUP)) {
    ret |= scheduler::EV_IO_CLOSE;
  }
  if (os & EPOLLERR) {
    ret |= scheduler::EV_IO_ERROR;
  }

  return ret;
}


} // anonymous namespace



void
scheduler::scheduler_impl::init_impl()
{
  m_epoll_fd = -1;

  m_epoll_fd = ::epoll_create1(EPOLL_CLOEXEC);
  if (-1 == m_epoll_fd) {
    switch (errno) {
      case EMFILE:
      case ENFILE:
        throw exception(ERR_NUM_FILES);
        break;

      case ENOMEM:
        throw exception(ERR_OUT_OF_MEMORY);
        break;

      default:
        throw exception(ERR_UNEXPECTED);
        break;
    }
  }
}



void
scheduler::scheduler_impl::deinit_impl()
{
  if (-1 != m_epoll_fd) {
    ::close(m_epoll_fd);
  }
}



void
scheduler::scheduler_impl::register_fd(int fd, uint64_t const & events)
{
  int fds[1] = { fd };
  register_fds(fds, 1, events);
}



void
scheduler::scheduler_impl::register_fds(int const * fds, size_t size,
    uint64_t const & events)
{
  int translated = translate_events_to_os(events);

  for (size_t i = 0 ; i < size ; ++i) {
    ::epoll_event event;
    event.events = translated;
    event.data.fd = fds[i];
    int ret = ::epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fds[i], &event);

    if (-1 == ret) {
      // TODO do better than this
      LOG("errno: " << errno);
    }
  }
}



void
scheduler::scheduler_impl::unregister_fd(int fd, uint64_t const & events)
{
  int fds[1] = { fd };
  unregister_fds(fds, 1, events);
}



void
scheduler::scheduler_impl::unregister_fds(int const * fds, size_t size,
    uint64_t const & events)
{
  // TODO
}



} // namespace packetflinger
