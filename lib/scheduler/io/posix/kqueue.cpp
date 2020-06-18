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

#include "kqueue.h"

#include <packeteer/error.h>
#include <packeteer/types.h>
#include <packeteer/scheduler/events.h>

#include "../../../globals.h"
#include "../../scheduler_impl.h"
#include "../../../thread/chrono.h"


// Posix
#include <unistd.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

#include <errno.h>

#include <vector>

namespace sc = std::chrono;

namespace packeteer::detail {

namespace {

inline int
translate_events_to_os(events_t const & events)
{
  switch (events) {
    case PEV_IO_READ:
      return EVFILT_READ;

    case PEV_IO_WRITE:
      return EVFILT_WRITE;
  }

  return -1;
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



inline void
set_event_if_selected(std::vector<struct kevent> & pending,
    size_t & pending_offset, handle const & handle,
    events_t const & events, events_t const & requested)
{
  if (!(events & requested)) {
    return;
  }

  int translated = translate_events_to_os(requested);

  EV_SET(&pending[pending_offset++], handle.sys_handle(), translated,
      EV_ADD, 0, 0, nullptr);
}


inline void
del_event_if_selected(std::vector<struct kevent> & pending,
    size_t & pending_offset, handle const & handle,
    events_t const & events, events_t const & requested)
{
  if (!(events & requested)) {
    return;
  }

  int translated = translate_events_to_os(requested);

  EV_SET(&pending[pending_offset++], handle.sys_handle(), translated,
      EV_DELETE, 0, 0, nullptr);
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
      set_event_if_selected(pending, pending_offset, handles[i],
          events, PEV_IO_READ);
      set_event_if_selected(pending, pending_offset, handles[i],
          events, PEV_IO_WRITE);
    }
    else {
      del_event_if_selected(pending, pending_offset, handles[i],
          events, PEV_IO_READ);
      del_event_if_selected(pending, pending_offset, handles[i],
          events, PEV_IO_WRITE);
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
            DLOG("Handle " << handles[i] << " [" << handles[i].sys_handle() \
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



inline void
modify_conn_set(int action, int queue, connector const * conns, size_t size,
    events_t const & events)
{
  // We treat error, etc. events as applying to both the read and write end of
  // each connector, but apply PEV_IO_READ only to the read end, and
  // PEV_IO_WRITE only to the write end. If both handles are the same, we let
  // the system figure that out.
  std::vector<handle> readers;
  readers.reserve(size);
  std::vector<handle> writers;
  writers.reserve(size);

  for (size_t i = 0 ; i < size ; ++i) {
    connector const & conn = conns[i];
    if (events & PEV_IO_READ) {
      readers.push_back(conn.get_read_handle());
    }
    if (events & PEV_IO_WRITE) {
      writers.push_back(conn.get_write_handle());
    }
  }

  modify_kqueue(action, queue, &readers[0], readers.size(),
      events | ~PEV_IO_WRITE);
  modify_kqueue(action, queue, &writers[0], writers.size(),
      events | ~PEV_IO_READ);
}



} // anonymous namespace



io_kqueue::io_kqueue(std::shared_ptr<api> api)
  : io(api)
  , m_kqueue_fd(-1)
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

  DLOG("KQueue based I/O subsystem created.");
}



io_kqueue::~io_kqueue()
{
  ::close(m_kqueue_fd);
  m_kqueue_fd = -1;
}



void
io_kqueue::register_connector(connector const & conn, events_t const & events)
{
  connector conns[1] = { conn };
  constexpr auto size = sizeof(conns) / sizeof(connector);

  io::register_connectors(conns, size, events);

  modify_conn_set(true, m_kqueue_fd, conns, size, events);
}



void
io_kqueue::register_connectors(connector const * conns, size_t size,
    events_t const & events)
{
  io::register_connectors(conns, size, events);

  modify_conn_set(true, m_kqueue_fd, conns, size, events);
}




void
io_kqueue::unregister_connector(connector const & conn, events_t const & events)
{
  connector conns[1] = { conn };
  constexpr auto size = sizeof(conns) / sizeof(connector);

  io::unregister_connectors(conns, size, events);

  modify_conn_set(false, m_kqueue_fd, conns, size, events);
}



void
io_kqueue::unregister_connectors(connector const * conns, size_t size,
    events_t const & events)
{
  io::register_connectors(conns, size, events);

  modify_conn_set(false, m_kqueue_fd, conns, size, events);
}




void
io_kqueue::wait_for_events(io_events & events,
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
      io_event data = {
        m_connectors[kqueue_events[i].ident],
        PEV_IO_ERROR
      };
      events.push_back(data);
    }
    else if (kqueue_events[i].flags & EV_EOF) {
      io_event data = {
        m_connectors[kqueue_events[i].ident],
        PEV_IO_CLOSE
      };
      events.push_back(data);
    }
    else {
      events_t translated = translate_os_to_events(kqueue_events[i].filter);
      if (translated >= 0) {
        io_event data = {
          m_connectors[kqueue_events[i].ident],
          translated
        };
        events.push_back(data);
      }
    }
  }
}



} // namespace packeteer::detail
