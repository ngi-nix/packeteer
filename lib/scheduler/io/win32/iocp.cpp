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

#include <liberate/types.h>

#include <packeteer/error.h>
#include <packeteer/scheduler/events.h>

#include "../../../globals.h"
#include "../../scheduler_impl.h"
#include "../../../thread/chrono.h"

#include "../../../win32/sys_handle.h"
#include "../../../connector/win32/io_operations.h"

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



inline void
unregister_from_read_events(connector & conn)
{
  if (conn.type() != CT_PIPE && conn.type() != CT_ANON) {
    return;
  }

  if (!conn.get_read_handle().sys_handle()->read_context.pending_io()) {
    return;
  }

  if (conn.get_read_handle().sys_handle()->read_context.schedlen > 0) {
    return;
  }

  DLOG("No longer interested when pipe-like handle is readable.");
  conn.get_read_handle().sys_handle()->read_context.cancel_io();
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

  DLOG("I/O completion port subsystem created.");
}



io_iocp::~io_iocp()
{
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
      unregister_from_read_events(*cn);
    }
  }

  // Pass to superclass
  io::unregister_connectors(conns, size, events);
}




void
io_iocp::wait_for_events(io_events & events,
      duration const & timeout)
{
  auto wait_ms = std::chrono::ceil<std::chrono::milliseconds>(timeout).count();
  DLOG("Wait for IOCP events: " << wait_ms << "ms");

  // Before waiting, we want to be sure that every connector that is registered
  // for read events actually has a read pending. If necessary, we schedule zero
  // byte reads to get them there. We're not interested in the results, but we
  // do want events from IOCP.
  for (auto & [sys_handle, events] : m_sys_handles) {
    if (!(events & PEV_IO_READ)) {
      continue;
    }


    if (!sys_handle->read_context.pending_io()) {
      DLOG("Request notification when pipe-like handle becomes readable.");
      auto & conn = m_connectors[sys_handle];
      zero_byte_read(conn.get_read_handle());
    }
  }

  // Wait for I/O completion.
  OVERLAPPED_ENTRY entries[PACKETEER_IOCP_MAXEVENTS] = {};
  ULONG read = 0;

  BOOL ret = GetQueuedCompletionStatusEx(m_iocp, entries,
      PACKETEER_IOCP_MAXEVENTS,
      &read,
      wait_ms,
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

  // First, go through actually received events and add them to the temporary
  // map.
  for (size_t i = 0 ; i < read ; ++i) {
    // Our OVERLAPPED are always io_context
    auto ctx = reinterpret_cast<detail::io_context *>(entries[i].lpOverlapped);

    // Find the connector for the system handle stored in the context.
    connector conn;
    for (auto & [sys_handle, c] : m_connectors) {
      if (sys_handle->handle == ctx->handle) {
        conn = c;
        break;
      }
    }
    if (!conn) {
      // XXX This seems to happen occasionally when an event completed, but
      //     hasn't been handled yet?
      if (ctx->handle != INVALID_HANDLE_VALUE) {
        DLOG("Got event on a handle " << ctx->handle << " that is not related to a known connector!");
      }
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

        case ERROR_IO_INCOMPLETE:
        case ERROR_IO_PENDING:
          // Just not done yet, no error.
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

      // TODO maybe report PEV_IO_CLOSE if ctx->type was PEV_IO_READ and
      //      num_transferred is zero?
    }

    DLOG("RAW Events for connector " << conn << " are " << ev);
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

    tmp_events[conn] |= PEV_IO_WRITE;
  }

  // Add all temporarily collected events to the out queue.
  for (auto & [conn, ev] : tmp_events) {
    if (ev) {
      DLOG("Final events for connector " << conn << " are " << ev);
      events.push_back({ conn, ev });
    }
  }

  DLOG("WIN32 IOCP got " << events.size() << " event entries to report.");
}



} // namespace packeteer::detail
