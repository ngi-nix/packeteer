/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2020 Jens Finkhaeuser.
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

#include "socket_select.h"

#include <packeteer/scheduler/events.h>

#include "../../../macros.h"
#include "../../../net/netincludes.h"
#include "../../../win32/sys_handle.h"

#include "../../../scheduler/scheduler_impl.h"

namespace packeteer::detail {

namespace {


inline events_t
event_or_error(WSANETWORKEVENTS const & input, int mask, int bit, events_t result)
{
  if (input.lNetworkEvents & mask) {
    if (input.iErrorCode[bit] == ERROR_SUCCESS) {
      return result;
    }
    return PEV_IO_ERROR;
  }
  return {};
}


inline events_t
translate_events(WSANETWORKEVENTS const & input)
{
  events_t events{};

  events |= event_or_error(input, FD_READ,    FD_READ_BIT,    PEV_IO_READ);
  events |= event_or_error(input, FD_WRITE,   FD_WRITE_BIT,   PEV_IO_WRITE);
  events |= event_or_error(input, FD_CONNECT, FD_CONNECT_BIT, PEV_IO_OPEN|PEV_IO_READ);
  events |= event_or_error(input, FD_ACCEPT,  FD_ACCEPT_BIT,  PEV_IO_OPEN|PEV_IO_READ);
  events |= event_or_error(input, FD_CLOSE,   FD_CLOSE_BIT,   PEV_IO_CLOSE);

  return events;
}



template <typename valueT>
inline void
remove(std::vector<valueT> & vec, size_t idx)
{
  // This code is pretty simple because we know that the value at the index is
  // invalid. What we do is, we swap it for the last value, and shrink the
  // vector by one. It's even simpler than that, because we only need to
  // preserve the last value.
  size_t last_idx = vec.size() - 1;
  if (idx != last_idx) {
    vec[idx] = vec[last_idx];
  }
  vec.resize(last_idx); // last_idx doubles as new size
}



inline void
add_socket(iocp_socket_select::socket_data & data, handle::sys_handle_t const & sock,
    int events)
{
  // New socket, so create event handle.
  auto ev = WSACreateEvent();
  if (ev == WSA_INVALID_EVENT) {
    ERRNO_LOG("Failed to create WSA event handle.");
    throw exception(ERR_UNEXPECTED, "Failed to create WSA event handle.");
  }

  // Push socket and event to the data.
  data.sockets.push_back(sock);
  data.events.push_back(ev);

  // Register handle for events.
  DLOG("Adding " << sock << " select events: " << events);
  auto err = WSAEventSelect(sock->socket, ev, events);
  if (err == SOCKET_ERROR) {
    ERRNO_LOG("WSAEventSelect failed.");
    throw exception(ERR_UNEXPECTED, "WSAEventSelect failed.");
  }
}



inline void
modify_socket(iocp_socket_select::socket_data & data, size_t idx, int events)
{
  // Modification is the simple part. Just find the associated event handle,
  // and update the events.
  auto & ev = data.events[idx];
  auto & sock = data.sockets[idx];

  DLOG("Modifying " << sock << " select events: " << events);
  auto err = WSAEventSelect(sock->socket, ev, events);
  if (err == SOCKET_ERROR) {
    ERRNO_LOG("WSAEventSelect failed.");
    throw exception(ERR_UNEXPECTED, "WSAEventSelect failed.");
  }
}



inline void
delete_socket(iocp_socket_select::socket_data & data, size_t idx)
{
  // First, stop listening for events. If errors occur here, we just ignore
  // them.
  auto & ev = data.events[idx];
  auto & sock = data.sockets[idx];

  DLOG("Unregistering " << sock << " from select events.");
  auto err = WSAEventSelect(sock->socket, ev, 0);
  if (err == SOCKET_ERROR) {
    ERRNO_LOG("WSAEventSelect failed, but we ignore that.");
  }

  // Next, close the handle.
  auto res = WSACloseEvent(ev);
  if (!res) {
    ERRNO_LOG("Could not close event handle, but we ignore that.");
  }

  // Free the slots
  data.events[idx] = WSA_INVALID_EVENT;
  data.sockets[idx] = handle::INVALID_SYS_HANDLE;

  // Since we don't know where in the vectors we freed the slots, we need to
  // compact them both.
  remove(data.events, idx);
  remove(data.sockets, idx);
}



} // anonymous namespace

iocp_socket_select::iocp_socket_select(connector & interrupt)
  : m_interrupt(interrupt)
  , m_sock_data(new socket_data{})
{
  // Internal handle for interrupting exiting the thread loop
  auto ev = WSACreateEvent();
  if (ev == WSA_INVALID_EVENT) {
    ERRNO_LOG("Failed to create WSA event handle.");
    throw exception(ERR_UNEXPECTED, "Failed to create WSA event handle.");
  }

  // Add handle with fake socket to the socket data.
  m_sock_data->events.push_back(ev);
  m_sock_data->sockets.push_back(handle::INVALID_SYS_HANDLE);

  // Start thread
  m_thread = std::thread([this]() -> void
  {
    this->run_loop();
  });
}



iocp_socket_select::~iocp_socket_select()
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Signal the thread to stop running.
    m_running = false;
    WSASetEvent(m_sock_data->events[0]);
  }

  m_thread.join();
  DLOG("IOCP select loop thread ended.");

  std::lock_guard<std::mutex> lock(m_mutex);

  // Close all events. We use the delete_socket function for most of them.
  for (size_t idx = 1 ; idx < m_sock_data->events.size() ; ++idx) {
    delete_socket(*m_sock_data, idx);
  }

  WSACloseEvent(m_sock_data->events[0]);

  delete m_sock_data;

  DLOG("IOCP select loop cleaned up.");
}



void
iocp_socket_select::configure_socket(handle::sys_handle_t const & sock, int events)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  // Find the socket in the vector
  ssize_t idx = -1;
  for (size_t i = 0 ; i < m_sock_data->sockets.size() ; ++i) {
    if (m_sock_data->sockets[i] == sock) {
      idx = i;
      break;
    }
  }
  if (idx < 0) {
    add_socket(*m_sock_data, sock, events);
  }
  else {
    // The socket exists. What are the events? This determines whether to
    // modify or delete the socket.
    if (events) {
      modify_socket(*m_sock_data, idx, events);
    }
    else {
      delete_socket(*m_sock_data, idx);
    }
  }

  // Finally, signal main loop
  WSASetEvent(m_sock_data->events[0]);
}



bool
iocp_socket_select::partial_loop_iteration(socket_data data, size_t offset, size_t size,
      DWORD timeout, bool & notify)
{
  // Wait for the window of events given in the offset/size parameters.
  // DLOG("IOCP socket select loop going to sleep.");
  auto ret = WSAWaitForMultipleEvents(size, &(data.events[offset]),
      FALSE, timeout, FALSE);
  // DLOG("IOCP socket select loop wakeup.");

  // If we're no longer running, exit immediately
  if (!m_running) {
    return false;
  }

  switch (ret) {
    case WSA_WAIT_IO_COMPLETION:
    case WSA_WAIT_TIMEOUT:
      // No events.
      return true;

    case WSA_WAIT_FAILED:
      ERRNO_LOG("WSAWaitForMultipleEvents() failed.");
      // It's unclear what we should do, so let's just continue.
      return true;

    default:
      break;
  }

  // Some events returned.
  auto first = ret - WSA_WAIT_EVENT_0;

  notify = false;
  for (auto idx = offset + first ; idx < offset + size ; ++idx) {
    // Internal event; just reset it fast and go on.
    auto & sock = data.sockets[idx];
    auto & ev = data.events[idx];

    if (idx == 0) {
      WSAResetEvent(ev);
      continue;
    }

    // Figure out which events were fired. Really, this is largely a moot
    // question due to its usage, but let's keep this code a tiny bit more
    // generically useful.
    WSANETWORKEVENTS events;
    auto ret = WSAEnumNetworkEvents(sock->socket, ev, &events);
    if (ret == SOCKET_ERROR) {
      ERRNO_LOG("Error enumerating network events; ignoring.");
    }

    // Queue the events
    result res = {
      sock,
      translate_events(events),
    };
    if (res.events) {
      collected_events.push(res);
      notify = true;
    }
  }

  return true;
}




void
iocp_socket_select::run_loop()
{
  // We keep running until the running flag is unset
  DLOG("IOCP socket select loop start.");

  // Copy event handle data. This is so that subsequent calls to
  // configure_socket() don't block, and we have the association of events to
  // sockets intact. We do this once before the loop, and then after every
  // wakeup - that way, we filter out irrelevant events from sockets that we're
  // no longer interested in since the last copying, and prepare the list
  // for the next wait.
  socket_data data;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    data = *m_sock_data;
  }

  while (m_running) {
    // We may be watching a smallish number of sockets, or a large one - due to
    // the limitations of the Win32 API, in the second case, we have to chop up
    // our events into windows of WSA_MAXIMUM_WAIT_EVENTS size, and wait for
    // each window separately. Of course if we do that, it makes no sense to
    // wait for a very long time, as we may miss events outside our current
    // window.
    if (data.events.size() <= WSA_MAXIMUM_WAIT_EVENTS) {
      // The simple case.
      bool notify = false;
      if (!partial_loop_iteration(data, 0, data.events.size(), WSA_INFINITE, notify)) {
        break;
      }

      // If we produced events, notify the main IOCP loop
      if (notify) {
        DLOG("Notifying IOCP loop of events.");
        interrupt(m_interrupt);
      }
    }
    else {
      bool exit_run_loop = false;

      size_t offset = 0;
      for ( ; offset < data.events.size() ; offset += WSA_MAXIMUM_WAIT_EVENTS) {
        size_t remaining = data.events.size() - offset;
        size_t size = std::min(size_t(WSA_MAXIMUM_WAIT_EVENTS), remaining);
        bool notify = false;
        // We want to go through the first windows really fast, and linger only
        // a little on the last window. This is a compromise between not waiting
        // delaying unnecessarily that events in later windows are checked, and
        // busy-looping all the time.
        // XXX: Maybe picking the window at which we're waiting randomly, or
        //      moving it forward deterministically would be the fairest
        //      approach?
        DWORD timeout = 0;
        if (offset + size >= data.events.size()) {
          // Last window
          timeout = PACKETEER_EVENT_WAIT_INTERVAL_USEC / 1000;
        }
        if (!partial_loop_iteration(data, offset, size, timeout, notify)) {
          exit_run_loop = true;
          break;
        }

        // If we produced events, notify the main IOCP loop
        if (notify) {
          DLOG("Notifying IOCP loop of events.");
          interrupt(m_interrupt);
        }
      }

      if (exit_run_loop) {
        break;
      }
    }

    // Update data for the next iteration
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      data = *m_sock_data;
    }
  }
  DLOG("IOCP socket select loop end.");
}


} // namespace packeteer::detail
