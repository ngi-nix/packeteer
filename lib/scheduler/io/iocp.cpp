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

#include <set>

#include <packeteer/error.h>
#include <packeteer/types.h>
#include <packeteer/scheduler/events.h>

#include "../../globals.h"
#include "../scheduler_impl.h"
#include "../../thread/chrono.h"

#include "../../win32/sys_handle.h"
#include "../../connector/win32/overlapped.h"

#include <vector>

namespace sc = std::chrono;

namespace packeteer::detail {

namespace {

inline bool
register_handle(HANDLE iocp, std::set<HANDLE> & associated, handle const & handle)
{
  ULONG_PTR completion_key = handle.hash();
  auto ret = CreateIoCompletionPort(handle.sys_handle()->handle,
      iocp, completion_key, 0);
  if (!ret) {
    switch (WSAGetLastError()) {
      case ERROR_INVALID_PARAMETER:
        {
          auto iter = associated.find(handle.sys_handle()->handle);
          if (iter != associated.end()) {
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

} // anonymous namespace


io_iocp::io_iocp()
  : m_iocp(INVALID_HANDLE_VALUE)
{
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

      if (!register_handle(m_iocp, m_associated, conn.get_write_handle().sys_handle())) {
        io::unregister_connector(conn, events);
        continue;
      }
      m_associated.insert(conn.get_write_handle().sys_handle()->handle);
    }

    // Either way, use the super class functionality to remember which events the
    // connector was registered for.
    io::register_connector(conn, events);

    // If we've registered the connector for read handle for read events, we
    // should schedule a PEEK... otherwise we won't be notified when a write
    // occurs.
    if (m_sys_handles[conn.get_read_handle().sys_handle()] & PEV_IO_READ) {
      char buf[PACKETEER_IO_BUFFER_SIZE];
      // TODO turn into PEEK
      size_t actually_read = 0;
      // FIXME don't const_cast, make non-const.
      auto cn = const_cast<connector *>(&conn);
      auto err = cn->read(buf, PACKETEER_IO_BUFFER_SIZE, actually_read);
      // XXX ignore result; if we're just peeking, it doesn't matter.
    }
  }
}




void
io_iocp::unregister_connector(connector const & conn, events_t const & events)
{
  // We can't actually unregister any conns - we can just filter here; the OS will
  // keep the connector associated with the IOCP.
  io::unregister_connector(conn, events);
}



void
io_iocp::unregister_connectors(connector const * conns, size_t size,
    events_t const & events)
{
  // We can't actually unregister any conns - we can just filter here; the OS will
  // keep the connector associated with the IOCP.
  io::unregister_connectors(conns, size, events);
}




void
io_iocp::wait_for_events(std::vector<event_data> & events,
      duration const & timeout)
{
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

  // Every handle is writable with OVERLAPPED I/O. We need to add a PEV_IO_WRITE
  // for every handle registered for write events. But we can also process events
  // we received.
  std::map<connector, events_t> tmp_events;
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
      DLOG("Got event on a handle that is not related to a known connector!");
      continue;
    }

    events_t ev = 0; // Nothing yet

    // Read the status - this now returns immediately, as the OVERLAPPED
    // structure contains everything the function needs.
    DWORD num_transferred = 0;
    BOOL res = GetOverlappedResult(ctx->handle, ctx, &num_transferred, FALSE);

    if (!res) {
      ERRNO_LOG("IOCP reports error for operation: " << static_cast<int>(ctx->type));
      ev |= PEV_IO_ERROR;
    }
    else {
      DLOG("CTX TYPE: " << (int) ctx->type);
      // The values of ctx->type have the same values as events_t
      ev |= ctx->type;

      // TODO maybe repoort PEV_IO_CLOSE if ctx->type was PEV_IO_READ and
      //      num_transferred is zero?
    }

    DLOG("Events for connector " << conn << " are " << ev);
    tmp_events[conn] = ev;
  }

  // The temporary events now hold all *actual* events. Let's now add a write
  // event for all valid and error free connectors that were *registered* for
  // write events, too.
  for (auto & [sys_handle, conn] : m_connectors) {
    if (!conn) {
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
