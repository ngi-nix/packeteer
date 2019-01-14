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
#include <packeteer/detail/io_kqueue.h>
#include <packeteer/detail/globals.h>
#include <packeteer/detail/scheduler_impl.h>

// Posix
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

#include <errno.h>

#include <vector>

#include <packeteer/error.h>
#include <packeteer/types.h>
#include <packeteer/events.h>
#include <packeteer/thread/chrono.h>

namespace sc = std::chrono;

namespace packeteer {
namespace detail {

namespace {

inline int
translate_events_to_os(events_t const & events)
{
  int ret = 0;

  if (events & PEV_IO_READ) {
    ret |= EVFILT_READ;
  }
  if (events & PEV_IO_WRITE) {
    ret |= EVFILT_WRITE;
  }

  return ret;
}



inline events_t
translate_os_to_events(int os)
{
  events_t ret = 0;

  if (os & EVFILT_READ) {
    ret |= PEV_IO_READ;
  }
  if (os & EVFILT_WRITE) {
    ret |= PEV_IO_WRITE;
  }

  return ret;
}



inline void
modify_kqueue(bool add, int queue, int const * fds, size_t amount,
    events_t const & events)
{
  // Get enough memory for the transaction
  std::vector<struct kevent> pending;
  pending.resize(amount);

  // Register FDs individually
  for (size_t i = 0 ; i < amount ; ++i) {
    // Now we access the last (i.e. the newly added) element of the vector.
    int translated = translate_events_to_os(events);
    if (add) {
      EV_SET(&pending[i], fds[i], translated, EV_ADD|EV_CLEAR||EV_RECEIPT, 0, 0, nullptr);
    }
    else {
      EV_SET(&pending[i], fds[i], translated, EV_DELETE, 0, 0, nullptr);
    }
  }

  // Now flush the events to the kqueue.
  while (true) {
    int res = kevent(queue, &pending[0], pending.size(), nullptr, 0, nullptr);
    if (res >= 0) {
      break;
    }

    switch (errno) {
      case EINTR:  // signal interrupt handling
        continue;

      case EACCES:
        throw exception(ERR_ACCESS_VIOLATION, errno);

      case EFAULT:
      case EINVAL:
      case EBADF:
      case ENOENT:
        throw exception(ERR_INVALID_OPTION, errno);

      case ENOMEM:
        throw exception(ERR_OUT_OF_MEMORY, errno, "OOM trying to modify kqueue events");


      case ESRCH:
      default:
        throw exception(ERR_UNEXPECTED, errno);
    }
  }
}


} // anonymous namespace



io_kqueue::io_kqueue()
  : m_kqueue_fd(-1)
{
  int res = ::kqueue();
  if (res < 0) {
    switch (errno) {
      case EMFILE:
      case ENFILE:
        throw exception(ERR_NUM_FILES, "Too many file descriptors to create kqueue descriptor.");
        break;

      case ENOMEM:
        throw exception(ERR_OUT_OF_MEMORY, "OOM in kqueue call.");
        break;

      default:
        throw exception(ERR_UNEXPECTED, errno);
        break;
    }
  }
  else {
    m_kqueue_fd = res;
  }

  LOG("KQueue based I/O subsystem created.");
}



io_kqueue::~io_kqueue()
{
  close(m_kqueue_fd);
  m_kqueue_fd = -1;
}



void
io_kqueue::register_fd(int fd, events_t const & events)
{
  int fds[1] = { fd };
  register_fds(fds, 1, events);
}



void
io_kqueue::register_fds(int const * fds, size_t size,
    events_t const & events)
{
  modify_kqueue(true, m_kqueue_fd, fds, size, events);
}




void
io_kqueue::unregister_fd(int fd, events_t const & events)
{
  int fds[1] = { fd };
  unregister_fds(fds, 1, events);
}



void
io_kqueue::unregister_fds(int const * fds, size_t size,
    events_t const & events)
{
  modify_kqueue(false, m_kqueue_fd, fds, size, events);
}




void
io_kqueue::wait_for_events(std::vector<event_data> & events,
      duration const & timeout)
{
  // The entire event queue is already in the kernel, all we need to do is check
  // if events have occurred.
  ::timespec ts;
  ::packeteer::thread::chrono::convert(timeout, ts);

  struct kevent kqueue_events[PACKETEER_KQUEUE_MAXEVENTS];
  int ret = -1;
  while (true) {
    ret = kevent(m_kqueue_fd, nullptr, 0, kqueue_events,
        PACKETEER_KQUEUE_MAXEVENTS, &ts);
    if (ret >= 0) {
      break;
    }

    // Error handling
    switch (errno) {
      case EINTR: // signal interrupt handling
        continue;

      case EACCES:
        throw exception(ERR_ACCESS_VIOLATION, errno);

      case EFAULT:
      case EINVAL:
      case EBADF:
      case ENOENT:
        throw exception(ERR_INVALID_OPTION, errno);

      case ENOMEM:
        throw exception(ERR_OUT_OF_MEMORY, "OOM trying to modify kqueue events");

      case ESRCH:
      default:
        throw exception(ERR_UNEXPECTED, errno);
    }
  }

  // Map events
  for (int i = 0 ; i < ret ; ++i) {
    if (kqueue_events[i].flags & EV_ERROR) {
      event_data ev;
      ev.fd = kqueue_events[i].ident;
      ev.events = PEV_IO_ERROR;
      events.push_back(ev);
    }
    else if (kqueue_events[i].flags & EV_EOF) {
      event_data ev;
      ev.fd = kqueue_events[i].ident;
      ev.events = PEV_IO_CLOSE;
      events.push_back(ev);
    }
    else {
      int translated = translate_os_to_events(kqueue_events[i].filter);
      if (translated) {
        event_data ev;
        ev.fd = kqueue_events[i].ident;
        ev.events = translated;
        events.push_back(ev);
      }
    }
  }
}



}} // namespace packeteer::detail
