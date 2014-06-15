/**
 * This file is part of packetflinger.
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
#include <packetflinger/detail/io_poll.h>
#include <packetflinger/detail/scheduler_impl.h>

// Posix
#include <poll.h>

#include <errno.h>

#include <packetflinger/error.h>
#include <packetflinger/types.h>
#include <packetflinger/events.h>


namespace packetflinger {
namespace detail {

namespace {

inline int
translate_events_to_os(events_t const & events)
{
  int ret = 0;

  if (events & EV_IO_READ) {
    ret |= POLLIN | POLLPRI;
  }
  if (events & EV_IO_WRITE) {
    ret |= POLLOUT;
  }
  if (events & EV_IO_CLOSE) {
    ret |= POLLRDHUP | POLLHUP;
  }
  if (events & EV_IO_ERROR) {
    ret |= POLLERR | POLLNVAL;
  }

  return ret;
}



inline events_t
translate_os_to_events(int os)
{
  events_t ret = 0;

  if ((os & POLLIN) || (os & POLLPRI)) {
    ret |= EV_IO_READ;
  }
  if (os & POLLOUT) {
    ret |= EV_IO_WRITE;
  }
  if ((os & POLLRDHUP) || (os & POLLHUP)) {
    ret |= EV_IO_CLOSE;
  }
  if ((os & POLLERR) || (os & POLLNVAL)) {
    ret |= EV_IO_ERROR;
  }

  return ret;
}

} // anonymous namespace



io_poll::io_poll()
{
  LOG("Poll based I/O subsystem created.");
}



void
io_poll::init()
{
}



void
io_poll::deinit()
{
}



void
io_poll::register_fd(int fd, events_t const & events)
{
  m_fds[fd] |= events;
}



void
io_poll::register_fds(int const * fds, size_t size,
    events_t const & events)
{
  for (int i = 0 ; i < size ; ++i) {
    m_fds[fds[i]] |= events;
  }
}




void
io_poll::unregister_fd(int fd, events_t const & events)
{
  auto iter = m_fds.find(fd);
  iter->second &= ~events;
  if (!iter->second) {
    m_fds.erase(iter);
  }
}



void
io_poll::unregister_fds(int const * fds, size_t size,
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
io_poll::wait_for_events(std::vector<event_data> & events,
      twine::chrono::nanoseconds const & timeout)
{
  // FIXME also set signal mask & handle it accordingly

  // Prepare FD set
  size_t size = m_fds.size();
  ::pollfd fds[size];

  int i = 0;
  for (auto entry : m_fds) {
    fds[i].fd = entry.first;
    fds[i].events = translate_events_to_os(entry.second);
    ++i;
  }

  // Wait for events
#if defined(PACKETFLINGER_HAVE_PPOLL)
  ::timespec ts;
  timeout.as(ts);

  int ret = ::ppoll(fds, size, &ts, nullptr);
#else
  int ret = ::poll(fds, size, timeout.as<twine::chrono::milliseconds>());
#endif

  // Error handling
  if (ret < 0) {
    switch (errno) {
      case EFAULT:
      case EINVAL:
        throw exception(ERR_INVALID_VALUE, "Bad file descriptor in poll set.");
        break;

      case EINTR:
        // FIXME add proper signal handling; until then, try again.
        break;

      case ENOMEM:
        throw exception(ERR_OUT_OF_MEMORY, "OOM in poll call.");
        break;

      default:
        throw exception(ERR_UNEXPECTED, errno);
        break;
    }
  }

  // Map events; we'll need to iterate over the available file descriptors again
  // (conceivably, we could just use the subset in the FD sets, but that uses
  // additional memory).
  for (size_t i = 0 ; i < size ; ++i) {
    int translated = translate_os_to_events(fds[i].revents);
    if (translated) {
      events.push_back({fds[i].fd, translated});
    }
  }
}



}} // namespace packetflinger::detail
