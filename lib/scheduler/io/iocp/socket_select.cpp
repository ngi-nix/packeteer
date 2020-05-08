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



void
iocp_socket_select::run_loop()
{
  // We keep running until the running flag is unset
  DLOG("IOCP socket select loop start.");
  while (m_running) {
    // Copy event handle data. This is so that subsequent calls to
    // configure_socket() don't block, and we have the association of events to
    // sockets intact.
    socket_data data;
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      data = *m_sock_data;
    }

    // At this point we may have slightly outdated event data, but that doesn't
    // matter. We can work with this.
    // DLOG("IOCP socket select loop going to sleep.");
    auto ret = WSAWaitForMultipleEvents(data.events.size(), &(data.events[0]),
        FALSE, WSA_INFINITE, FALSE);
    // DLOG("IOCP socket select loop wakeup.");

    // If we're no longer running, exit immediately
    if (!m_running) {
      break;
    }

    switch (ret) {
      case WSA_WAIT_IO_COMPLETION:
      case WSA_WAIT_TIMEOUT:
        // No events.
        continue;

      default:
        break;
    }

    // Some events returned.
    auto first = ret - WSA_WAIT_EVENT_0;

    bool notify = false;
    for (auto idx = first ; idx < data.events.size() ; ++idx) {
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

    // If we produced events, notify the main IOCP loop
    if (notify) {
      DLOG("Notifying IOCP loop of events.");
      interrupt(m_interrupt);
    }
  }
  DLOG("IOCP socket select loop end.");
}


} // namespace packeteer::detail
