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


io_iocp::io_iocp()
  : m_iocp(INVALID_HANDLE_VALUE)
{
  HANDLE h = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, NULL, 0);
  if (h == INVALID_HANDLE_VALUE) {
    throw exception(ERR_UNEXPECTED, WSAGetLastError(),
        "Could not create I/O completion port");
  }

  m_iocp = h;
  LOG("I/O completion port subsystem created.");
}



io_iocp::~io_iocp()
{
  CloseHandle(m_iocp);
  m_iocp = INVALID_HANDLE_VALUE;
}



void
io_iocp::register_handle(handle const & handle, events_t const & events)
{
  LOG("REG #1: " << handle);
  struct handle handles[1] = { handle };
  register_handles(handles, 1, events);
}



void
io_iocp::register_handles(handle const * handles, size_t size,
    events_t const & events)
{
  for (size_t i = 0 ; i < size ; ++i) {
    handle const & handle = handles[i];
  LOG("REG #2: " << handle);

    // Try to find the handle in our registered set.
    auto iter = m_registered.find(handle);
    if (iter == m_registered.end()) {
      // New handle.
      LOG("Associate handle " << handle << " with I/O completion port.");
      LOG("HANDLE HASH "<< handle.hash());
      ULONG_PTR f = handle.hash();
      LOG("ULONGD "<<f);
      auto ret = CreateIoCompletionPort(handle.sys_handle()->handle,
          m_iocp, handle.hash(), 0);
      if (!ret) {
        ERRNO_LOG("Failed to associate handle " << handle << " with I/O completion port!");
        continue;
      }
      m_registered[handle] = events;
    }
    else {
      // Existing handle; just modify the event set.
      LOG("Update events on previously registered handle " << handle);
      iter->second |= events;
    }
  }
}




void
io_iocp::unregister_handle(handle const & handle, events_t const & events)
{
  struct handle handles[1] = { handle };
  unregister_handles(handles, 1, events);
}



void
io_iocp::unregister_handles(handle const * handles, size_t size,
    events_t const & events)
{
  // We can't actually unregister any handles - we can just filter here; the OS will
  // keep the handle associated with the IOCP.
  for (size_t i = 0 ; i < size ; ++i) {
    handle const & handle = handles[i];

    // Find handle. If it doesn't exist, just ignore it.
    auto iter = m_registered.find(handle);
    if (iter == m_registered.end()) {
      continue;
    }

    // So we found it - unset all the flags we received.
    iter->second |= ~events;
    LOG("Updated handle events.");

    // If there are no more events now, we can remove the entire entry.
    if (!iter->second) {
      LOG("Erased handle complete from registered set.");
      m_registered.erase(iter);
    }
  }
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
      std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count(),
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
  LOG("Dequeued " << read << " I/O events.");

  // FIXME - we're never getting read events, because no reads have been scheduled.
  //       - we can fix that by preemptively scheduling a read for every handle
  //         registered for reading...
  //       - HOWEVER, because the send/receive/etc. functionality may be different
  //         for different handles, this means we need to work on connector, not
  //         on handle granularity. That's fine, really, and we can make handles
  //         disappear entirely in the public interface.

  // Every handle is writable with OVERLAPPED I/O. We need to add a PEV_IO_WRITE
  // for every handle registered for write events. But we can also process events
  // we received.
  for (auto & [registered, registered_events] : m_registered) {
    LOG("Processing events for handle " << registered);
    event_data data = {
      registered,
      0, // Nothing for now.
    };

    // The handle is registered for write events, so report that we got one.
    if (registered_events & PEV_IO_WRITE) {
      data.m_events |= PEV_IO_WRITE;
    }

    // Also check for real events.
    for (size_t i = 0 ; i < read ; ++i) {
      handle_key_t key = entries[i].lpCompletionKey;
      LOG("Got event " << i << " for handle " << key);
      // FIXME if (registered.hash() != key) {
      // FIXME   continue;
      // FIXME }

      // Looks like we have an event on this handle. Let's see what the
      // handle is registered for.
      LOG("OVERLAPPED: " << entries[i].lpOverlapped);
      detail::overlapped::io_context * ctx = reinterpret_cast<
          detail::overlapped::io_context *
        >(entries[i].lpOverlapped);
      LOG("SYSTEM HANDLE: " << ctx->handle);
      LOG("Handle was registered for " << ctx->type);
      // TODO
      // -> Add all that had ANY event to the marked_writable set
      //    - If not in response to a write OVERLAPPED, mark the handle as
      //      writable (that's overlapped for you).
      //    - afterwards, mark all handles without events also as 
      //      writable.
    }

    // Now if the event data contains any event flags, we want to return
    // it.
    if (data.m_events) {
      events.push_back(data);
    }
  }

  LOG("Got " << events.size() << " event entries to report.");
#if 0
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
#endif
}



} // namespace packeteer::detail
