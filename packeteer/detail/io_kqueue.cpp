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
#include <unistd.h>
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
  switch (os) {
    case EVFILT_READ:
      return PEV_IO_READ;

    case EVFILT_WRITE:
      return PEV_IO_WRITE;
  }

  return -1;
}



inline bool
modify_kqueue(bool add, int queue, handle const * handles, size_t amount,
    events_t const & events)
{
  // Get enough memory for the transaction
  std::vector<struct kevent> pending;
  pending.resize(amount * 2);
  size_t pending_offset = 0;

  // Register FDs individually
  for (size_t i = 0 ; i < amount ; ++i) {
    // Now we access the last (i.e. the newly added) element of the vector.
    if (add) {
      if (events & PEV_IO_READ) {
        EV_SET(&pending[pending_offset++], handles[i].sys_handle(), EVFILT_READ,
            EV_ADD|EV_CLEAR||EV_RECEIPT, 0, 0, nullptr);
      }
      if (events & PEV_IO_WRITE) {
        EV_SET(&pending[pending_offset++], handles[i].sys_handle(), EVFILT_WRITE,
            EV_ADD|EV_CLEAR||EV_RECEIPT, 0, 0, nullptr);
      }
    }
    else {
      if (events & PEV_IO_READ) {
        EV_SET(&pending[pending_offset++], handles[i].sys_handle(), EVFILT_READ,
            EV_DELETE, 0, 0, nullptr);
      }
      if (events & PEV_IO_WRITE) {
        EV_SET(&pending[pending_offset++], handles[i].sys_handle(), EVFILT_WRITE,
            EV_DELETE, 0, 0, nullptr);
      }
    }
  }

  // Now flush the events to the kqueue.
  while (true) {
    int res = kevent(queue, &pending[0], pending_offset, nullptr, 0, nullptr);
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
        throw exception(ERR_INVALID_OPTION, errno);

      case ENOENT:
        // This happens if an FD has already been deleted. Since the FD set
        // can include more than one, it's difficult to understand how to
        // recover, because we don't know which FD triggered the issue.
        // So we'll try the entire set one by one.
        if (amount == 1) {
          // Already handling a single FD, so we can just bail. The return
          // value is largely for logging (below).
          return false;
        }
        for (size_t i = 0 ; i < amount ; ++i) {
          bool ret = modify_kqueue(add, queue, &handles[i], 1, events);
          if (!ret) {
            LOG("Handle " << handles[i] << " [" << handles[i].sys_handle() \
                << "] could not be modified, maybe it's a double delete?");
          }
        }
        return true;

      case ENOMEM:
        throw exception(ERR_OUT_OF_MEMORY, errno, "OOM trying to modify kqueue events");


      case ESRCH:
      default:
        throw exception(ERR_UNEXPECTED, errno);
    }
  }
  return true;
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
  ::close(m_kqueue_fd);
  m_kqueue_fd = -1;
}



void
io_kqueue::register_handle(handle const & handle, events_t const & events)
{
  struct handle handles[1] = { handle };
  register_handles(handles, 1, events);
}



void
io_kqueue::register_handles(handle const * handles, size_t size,
    events_t const & events)
{
  modify_kqueue(true, m_kqueue_fd, handles, size, events);
}




void
io_kqueue::unregister_handle(handle const & handle, events_t const & events)
{
  struct handle handles[1] = { handle };
  unregister_handles(handles, 1, events);
}



void
io_kqueue::unregister_handles(handle const * handles, size_t size,
    events_t const & events)
{
  modify_kqueue(false, m_kqueue_fd, handles, size, events);
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
      event_data data = {
        handle(kqueue_events[i].ident),
        PEV_IO_ERROR
      };
      events.push_back(data);
    }
    else if (kqueue_events[i].flags & EV_EOF) {
      event_data data = {
        handle(kqueue_events[i].ident),
        PEV_IO_CLOSE
      };
      events.push_back(data);
    }
    else {
      events_t translated = translate_os_to_events(kqueue_events[i].filter);
      if (translated >= 0) {
        event_data data = {
          handle(kqueue_events[i].ident),
          translated
        };
        events.push_back(data);
      }
    }
  }
}



}} // namespace packeteer::detail
