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

#include "poll.h"

#include <packeteer/error.h>

#include "../../scheduler_impl.h"
#include "../../../thread/chrono.h"

// Posix
#include <poll.h>

#include <errno.h>

#include <chrono>


namespace sc = std::chrono;

namespace packeteer::detail {

namespace {

inline int
translate_events_to_os(events_t const & events)
{
  int ret = 0;

  if (events & PEV_IO_READ) {
    ret |= POLLIN | POLLPRI;
  }
  if (events & PEV_IO_WRITE) {
    ret |= POLLOUT;
  }
  if (events & PEV_IO_CLOSE) {
    ret |= POLLHUP;
#if defined(PACKETEER_HAVE_POLLRDHUP)
    ret |= POLLRDHUP;
#endif
  }
  if (events & PEV_IO_ERROR) {
    ret |= POLLERR | POLLNVAL;
  }

  return ret;
}



inline events_t
translate_os_to_events(int os)
{
  events_t ret = 0;

  if ((os & POLLIN) || (os & POLLPRI)) {
    ret |= PEV_IO_READ;
  }
  if (os & POLLOUT) {
    ret |= PEV_IO_WRITE;
  }
  if ((os & POLLHUP)) {
    ret |= PEV_IO_CLOSE;
  }
#if defined(PACKETEER_HAVE_POLLRDHUP)
  if ((os & POLLRDHUP)) {
    ret |= PEV_IO_CLOSE;
  }
#endif
  if ((os & POLLERR) || (os & POLLNVAL)) {
    ret |= PEV_IO_ERROR;
  }

  return ret;
}

} // anonymous namespace



io_poll::io_poll(std::shared_ptr<api> api)
  : io(api)
{
  DLOG("Poll based I/O subsystem created.");
}



io_poll::~io_poll()
{
}



void
io_poll::wait_for_events(io_events & events,
      duration const & timeout)
{
  auto before = clock::now();
  auto cur_timeout = timeout;

  // Prepare FD set
  size_t size = m_sys_handles.size();
  std::vector<::pollfd> fds;
  fds.resize(size);

  size_t idx = 0;
  for (auto entry : m_sys_handles) {
    fds[idx].fd = entry.first;
    fds[idx].events = translate_events_to_os(entry.second);
    fds[idx].revents = 0;
    ++idx;
  }

  // Wait for events
  while (cur_timeout.count() > 0) {
#if defined(PACKETEER_HAVE_PPOLL)
    ::timespec ts;
    ::packeteer::thread::chrono::convert(cur_timeout, ts);

    int ret = ::ppoll(&fds[0], size, &ts, nullptr);
#else
    int ret = ::poll(&fds[0], size,
        sc::ceil<sc::milliseconds>(cur_timeout).count());
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
        continue;

      case EFAULT:
      case EINVAL:
        throw exception(ERR_INVALID_VALUE, errno, "Bad file descriptor in poll "
            "set.");
        break;

      case ENOMEM:
        throw exception(ERR_OUT_OF_MEMORY, errno, "OOM in poll call.");
        break;

      default:
        throw exception(ERR_UNEXPECTED, errno);
        break;
    }
  }

  // Map events; we'll need to iterate over the available file descriptors again
  // (conceivably, we could just use the subset in the FD sets, but that uses
  // additional memory).
  for (idx = 0 ; idx < size ; ++idx) {
    events_t translated = translate_os_to_events(fds[idx].revents);
    if (translated) {
      io_event ev = {
        m_connectors[fds[idx].fd],
        translated
      };
      events.push_back(ev);
    }
  }
}



} // namespace packeteer::detail
