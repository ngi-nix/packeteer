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

#include "iocp.h"

#include <unordered_set>

#include <packeteer/error.h>
#include <packeteer/types.h>
#include <packeteer/scheduler/events.h>

#include "../../globals.h"
#include "../scheduler_impl.h"
#include "../../thread/chrono.h"

#include "../../win32/sys_handle.h"
#include "../../connector/win32/overlapped.h"

#include "iocp/socket_select.h"

#include <vector>

namespace sc = std::chrono;

namespace packeteer::detail {

namespace {

/**
 * Register handle with IOCP.
 **/
inline bool
register_handle(HANDLE iocp, std::unordered_set<HANDLE> & associated, handle const & handle)
{
  DLOG("Supposed to register with IOCP: " << handle << " / " << handle.sys_handle()->handle);
  ULONG_PTR completion_key = handle.hash();
  auto ret = CreateIoCompletionPort(handle.sys_handle()->handle,
      iocp, completion_key, 0);
  if (!ret) {
    switch (WSAGetLastError()) {
      case ERROR_INVALID_PARAMETER:
        {
          auto iter = associated.find(handle.sys_handle()->handle);
          if (iter != associated.end()) {
            DLOG("Ignoring error, already registered.");
            return true; // Already associated
          }
        }
        // Fall through

      default:
        ERRNO_LOG("Failed to associate handle " << handle << " with I/O completion port!");
        return false;
    }
  }
  return true;
}



/**
 * Unregister connector for READ events, based on the connector type.
 **/
inline void
unregister_socket_from_read_events(iocp_socket_select & sock_select, connector & conn)
{
  DLOG("No longer interested when socket-like handle is readable.");
  sock_select.configure_socket(conn.get_read_handle().sys_handle(), 0);
}



inline void
unregister_pipe_from_read_events(connector & conn)
{
  DLOG("No longer interested when pipe-like handle is readable.");
  auto & manager = conn.get_read_handle().sys_handle()->overlapped_manager;
  manager.cancel_reads();
}



inline void
unregister_from_read_events(iocp_socket_select & sock_select, connector & conn)
{
  // Distinguish between socket-like and pipe-like connectors.
  // XXX See register.
  switch (conn.type()) {
    case CT_TCP4:
    case CT_TCP6:
    case CT_TCP:
    case CT_UDP4:
    case CT_UDP6:
    case CT_UDP:
    case CT_LOCAL:
      return unregister_socket_from_read_events(sock_select, conn);

    case CT_PIPE:
    case CT_ANON:
      return unregister_pipe_from_read_events(conn);

    case CT_UNSPEC:
    case CT_FIFO:
    case CT_USER:
      throw exception(ERR_INVALID_VALUE, "Connector of type "
          + std::to_string(conn.type()) + " currently not supported by IOCP; "
          "see https://gitlab.com/interpeer/packeteer/-/issues/12");
  }
}



/**
 * Register connector for READ events, based on the connector type.
 **/
inline void
register_socket_for_read_events(iocp_socket_select & sock_select, connector & conn)
{
  DLOG("Request notification when socket-like handle becomes readable.");
  sock_select.configure_socket(conn.get_read_handle().sys_handle(),
      FD_ACCEPT | FD_CONNECT | FD_READ | FD_CLOSE);
}



inline void
register_pipe_for_read_events(connector & conn)
{
  // Every read handle should have a pending read on it, so the system
  // notifies us when something is actually written on the other end. We'll
  // ask the overlapped manager if there is anything pending.
  bool schedule_read = conn.get_read_handle().sys_handle()->overlapped_manager.read_scheduled() < 0;
  if (schedule_read) {
    DLOG("Request notification when pipe-like handle becomes readable.");
    // Schedule a read of size 0 - this will result in win32 scheduling
    // overlapped I/O, but we never expect results.
    size_t actually_read = 0;
    conn.read(nullptr, 0, actually_read);
  }
}



inline void
register_for_read_events(iocp_socket_select & sock_select, connector & conn)
{
  // Distinguish between socket-like and pipe-like connectors.
  // XXX This is a highly non-portable hack, and will make life difficult for
  //     extensions. However, in the interest of getting a version out, this
  //     is what we'll do for now. See the related issue for fixing it:
  //     https://gitlab.com/interpeer/packeteer/-/issues/12
  switch (conn.type()) {
    case CT_TCP4:
    case CT_TCP6:
    case CT_TCP:
    case CT_UDP4:
    case CT_UDP6:
    case CT_UDP:
    case CT_LOCAL:
      return register_socket_for_read_events(sock_select, conn);

    case CT_PIPE:
    case CT_ANON:
      return register_pipe_for_read_events(conn);

    case CT_UNSPEC:
    case CT_FIFO:
    case CT_USER:
      throw exception(ERR_INVALID_VALUE, "Connector of type "
          + std::to_string(conn.type()) + " currently not supported by IOCP; "
          "see https://gitlab.com/interpeer/packeteer/-/issues/12");
  }
}

} // anonymous namespace


io_iocp::io_iocp(std::shared_ptr<api> const & api)
  : io(api)
{
  // Create IOCP
  HANDLE h = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, NULL, 0);
  if (h == INVALID_HANDLE_VALUE) {
    throw exception(ERR_UNEXPECTED, WSAGetLastError(),
        "Could not create I/O completion port");
  }
  m_iocp = h;

  // Register own interrupt connector
  m_interrupt = connector{m_api, "anon://"};
  error_t err = m_interrupt.connect();
  if (ERR_SUCCESS != err) {
    throw exception(err, "Could not connect select loop pipe.");
  }
  DLOG("Select loop pipe is " << m_interrupt);
  register_connector(m_interrupt, PEV_IO_READ | PEV_IO_ERROR | PEV_IO_CLOSE);

  // Create socket selection subsystem.
  m_sock_select = new iocp_socket_select{m_interrupt};

  DLOG("I/O completion port subsystem created.");
}



io_iocp::~io_iocp()
{
  delete m_sock_select;
  m_sock_select = nullptr;

  DLOG("Closing IOCP handle.");
  CloseHandle(m_iocp);
  m_iocp = INVALID_HANDLE_VALUE;
}



void
io_iocp::register_connector(connector const & conn, events_t const & events)
{
  connector conns[1] = { conn };
  register_connectors(conns, 1, events);
}



void
io_iocp::register_connectors(connector const * conns, size_t size,
    events_t const & events)
{
  for (size_t i = 0 ; i < size ; ++i) {
    connector const & conn = conns[i];

    DLOG("Registering connector " << conn << " for events " << events);

    // Try to find the connector in our registered set. If it's not yet found, we
    // have to add it to the IOCP.
    // XXX an implementation detail of the super class is that it does not matter
    //     if the connector was added for read or write events; both handles will
    //     be in the map.
    auto iter = m_connectors.find(conn.get_read_handle().sys_handle());
    if (iter == m_connectors.end()) {
      // New handle!
      if (!register_handle(m_iocp, m_associated, conn.get_read_handle().sys_handle())) {
        io::unregister_connector(conn, events);
        continue;
      }
      m_associated.insert(conn.get_read_handle().sys_handle()->handle);

      if (conn.get_read_handle() != conn.get_write_handle()) {
        if (!register_handle(m_iocp, m_associated, conn.get_write_handle().sys_handle())) {
          io::unregister_connector(conn, events);
          continue;
        }
        m_associated.insert(conn.get_write_handle().sys_handle()->handle);
      }
    }

    // Either way, use the super class functionality to remember which events the
    // connector was registered for.
    io::register_connector(conn, events);

    // Ensure we get READ events on the read handle, if that was requested.
    if (m_sys_handles[conn.get_read_handle().sys_handle()] & PEV_IO_READ) {
      auto cn = const_cast<connector *>(&conn);
      register_for_read_events(*m_sock_select, *cn);
    }
  }
}




void
io_iocp::unregister_connector(connector const & conn, events_t const & events)
{
  connector conns[1] = { conn };
  unregister_connectors(conns, 1, events);
}




void
io_iocp::unregister_connectors(connector const * conns, size_t size,
    events_t const & events)
{
  for (size_t i = 0 ; i < size ; ++i) {
    connector const & conn = conns[i];
    DLOG("Unregistering connector " << conn << " from events " << events);

    // If the connector is registered for READ, we can unregister it.
    if (m_sys_handles[conn.get_read_handle().sys_handle()] & PEV_IO_READ) {
      auto cn = const_cast<connector *>(&conn);
      unregister_from_read_events(*m_sock_select, *cn);
    }
  }

  // Pass to superclass
  io::unregister_connectors(conns, size, events);
}




void
io_iocp::wait_for_events(std::vector<event_data> & events,
      duration const & timeout)
{
  DLOG("Wait for IOCP events.");

  // Wait for I/O completion.
  OVERLAPPED_ENTRY entries[PACKETEER_IOCP_MAXEVENTS] = {};
  ULONG read = 0;

  BOOL ret = GetQueuedCompletionStatusEx(m_iocp, entries,
      PACKETEER_IOCP_MAXEVENTS,
      &read,
      std::chrono::ceil<std::chrono::milliseconds>(timeout).count(),
      TRUE);

  if (!ret) {
    switch (WSAGetLastError()) {
      case WAIT_TIMEOUT:
        // XXX What the fuck, win32? There was a timeout and you report a
        //     completion packet?
        // Set to zero, so the below doesn't break due to dereferencing
        // uninitialized OVERLAPPED_ENTRY.
        read = 0;
        break;

      default:
        ERRNO_LOG("Could not dequeue I/O events");
        return;
    }
  } 
  DLOG("Dequeued " << read << " I/O events.");

  // Temporary events container
  std::unordered_map<connector, events_t> tmp_events;

  // Grab events from select loop. This may be over very soon, if there were no
  // events detected there.
  iocp_socket_select::result sock_events;
  while (m_sock_select->collected_events.pop(sock_events)) {
    // See if we can find a connector for the socket.
    auto iter = m_connectors.find(sock_events.socket);
    if (iter == m_connectors.end()) {
      ELOG("Could not find connector for socket: " << sock_events.socket);
      continue;
    }
    tmp_events[iter->second] = sock_events.events;
  }
  DLOG("Collected " << tmp_events.size() << " socket events.");

  // First, go through actually received events and add them to the temporary
  // map.
  for (size_t i = 0 ; i < read ; ++i) {
    // Our OVERLAPPED are always io_context
    detail::overlapped::io_context * ctx = reinterpret_cast<
        detail::overlapped::io_context *
      >(entries[i].lpOverlapped);

    // Find the connector for the system handle stored in the context.
    connector conn;
    for (auto & [sys_handle, c] : m_connectors) {
      if (sys_handle->handle == ctx->handle) {
        conn = c;
        break;
      }
    }
    if (!conn) {
      // FIXME I'm not sure why this happens. The event was handled successfully
      //       in overlapped manager or its usage, but it seems to pop up here
      //       anyway.
      if (ctx->handle != INVALID_HANDLE_VALUE) {
        DLOG("Got event on a handle " << ctx->handle << " that is not related to a known connector!");
      }
      continue;
    }

    // If the connector is our own interrupt, clear it and continue.
    if (conn == m_interrupt) {
      clear_interrupt(m_interrupt);
      continue;
    }

    events_t ev = 0; // Nothing yet

    // Read the status - this now returns immediately, as the OVERLAPPED
    // structure contains everything the function needs.
    DWORD num_transferred = 0;
    BOOL res = GetOverlappedResult(ctx->handle, ctx, &num_transferred, FALSE);

    if (!res) {
      switch (WSAGetLastError()) {
        case ERROR_OPERATION_ABORTED:
          // Arguably, this is not an error. It just means the user decided
          // otherwise.
          break;

        default:
          ERRNO_LOG("IOCP reports error for operation: " << static_cast<int>(ctx->type));
          ev |= PEV_IO_ERROR;
          break;
      }
    }
    else {
      // The values of ctx->type have the same values as events_t
      ev |= ctx->type;

      // Since PEV_IO_OPEN is special to some platforms, automatically
      // mark every open connector as writable.
      if (ev & PEV_IO_OPEN && m_sys_handles[conn.get_write_handle().sys_handle()] & PEV_IO_WRITE) {
        ev |= PEV_IO_WRITE;
      }
      if (ev & PEV_IO_OPEN && m_sys_handles[conn.get_write_handle().sys_handle()] & PEV_IO_READ) {
        ev |= PEV_IO_READ;
      }


      // TODO maybe repoort PEV_IO_CLOSE if ctx->type was PEV_IO_READ and
      //      num_transferred is zero?
    }

    DLOG("Events for connector " << conn << " are " << ev);
    tmp_events[conn] |= ev;
  }

  // The temporary events now hold all *actual* events. Let's now add a write
  // event for all valid and error free connectors that were *registered* for
  // write events, too.
  for (auto & [sys_handle, conn] : m_connectors) {
    if (!conn || !conn.communicating()) {
      continue;
    }

    auto ev = m_sys_handles[sys_handle];
    if (!(ev & PEV_IO_WRITE)) {
      continue;
    }

    auto iter = tmp_events.find(conn);
    if (iter == tmp_events.end()) {
      tmp_events[conn] = PEV_IO_WRITE;
    }
    else if (!(iter->second & PEV_IO_ERROR)) {
      iter->second |= PEV_IO_WRITE;
    }
  }

  // Add all temporarily collected events to the out queue.
  for (auto & [conn, ev] : tmp_events) {
    DLOG("Final events for connector " << conn << " are " << ev);
    events.push_back({ conn, ev });
  }

  DLOG("Got " << events.size() << " event entries to report.");
}



} // namespace packeteer::detail
