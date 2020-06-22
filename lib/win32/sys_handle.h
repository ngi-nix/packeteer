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
#ifndef PACKETEER_SYS_HANDLE_H
#define PAKCETEER_SYS_HANDLE_H

#include <packeteer/handle.h>

#include <packeteer/scheduler/events.h>
#include <packeteer/net/socket_address.h>

#include "../macros.h"

namespace packeteer {

namespace detail {

/**
 * Overlapped operation types.
 *
 * Re-uses some PEV_IO_* values, because that simplifies the scheduler somewhat.
 * XXX: see scheduler/io/iocp.cpp
 */
enum PACKETEER_PRIVATE io_type : uint8_t
{
  CONNECT     = PEV_IO_OPEN,
  READ        = PEV_IO_READ,
  WRITE       = PEV_IO_WRITE,
};


/**
 * A context may be unused or pending.
 */
enum PACKETEER_PRIVATE io_state : int8_t
{
  UNUSED  = 0,
  PENDING = 1,
};


/**
 * Overlapped context
 */
struct PACKETEER_PRIVATE io_context : public OVERLAPPED
{
  io_state  state = UNUSED;

  // Either HANDLE or SOCKET is used.
  // We need to keep this here for the IOCP loop to associate OVERLAPPED
  // results back to a connector. It's static, though, and won't change.
  const union {
    HANDLE handle = INVALID_HANDLE_VALUE;
    SOCKET socket;
  };

  io_type    type = CONNECT;        // Buffers are only used for READ/WRITE

  void *     buf = nullptr;         // Reserved buffer for this context.
  size_t     schedlen = 0;          // *used* aka scheduled length of the
                                    // buffer - we may allocate more than we use.

  net::socket_address address = {}; // For datagram operations.


  inline io_context(HANDLE h)
    : OVERLAPPED()
    , handle(h)
  {
  }



  ~io_context()
  {
    cancel_io();
    state = UNUSED;
    handle = INVALID_HANDLE_VALUE;
    delete [] buf;
  }



  inline bool pending_io()
  {
    return state == PENDING;
  }



  inline void cancel_io()
  {
    if (!pending_io()) {
      return;
    }

    if (handle == INVALID_HANDLE_VALUE) {
      state = UNUSED;
      return;
    }

    auto ret = CancelIoEx(handle, this);
    if (!ret) {
      ERRNO_LOG("Unexpected error cancelling I/O operations on " << handle);
    }
    state = UNUSED;
  }



  inline void start_io(HANDLE h, io_type _type)
  {
    handle = h;
    state = PENDING;
    type = _type;
  }



  inline void start_io(SOCKET s, io_type _type)
  {
    socket = s;
    state = PENDING;
    type = _type;
  }



  inline void finish_io()
  {
    state = UNUSED;
  }



  inline void allocate(ssize_t amount)
  {
    if (amount <= schedlen) {
      schedlen = amount;
      return;
    }

    if (buf) {
      delete [] buf;
    }

    if (amount > 0) {
      schedlen = amount;
      buf = new char[amount];
    }
    else {
      schedlen = 0;
      buf = new char[1]; // Still need some address.
    }
  }
};


} // namespace detail


/**
 * Because we map all I/O to overlapped functions, a handle is more than the
 * WIN32 HANDLE. It includes a blocking flag, so we can simulate blocking
 * operations. And it includes an overlapped::manager, for handling the
 * allocation and use of OVERLAPPED structures.
 */
struct handle::opaque_handle
{
  // Either HANDLE or SOCKET is used.
  union {
    HANDLE            handle = INVALID_HANDLE_VALUE;
    SOCKET            socket;
  };

  bool                blocking = true;

  detail::io_context  write_context;
  detail::io_context  read_context;


  inline opaque_handle(HANDLE h = INVALID_HANDLE_VALUE)
    : handle(h)
    , write_context(handle)
    , read_context(handle)
  {
  }



  inline opaque_handle(SOCKET s)
    : socket(s)
    , write_context(handle)
    , read_context(handle)
  {
  }
};

} // namespace packeteer

#endif // guard
