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

#include "io_operations.h"

#include "../../macros.h"
#include "../../win32/sys_handle.h"

namespace packeteer::detail::io {

namespace {

inline error_t
translate_error()
{
  auto err = WSAGetLastError();
  switch (err) {
    case ERROR_IO_PENDING:
    case ERROR_IO_INCOMPLETE: // GetOverlappedResultEx()
    // case WSA_IO_PENDING: == ERROR_IO_PENDING
    case WSAEINPROGRESS:
    case WAIT_TIMEOUT:
      // Perfectly fine, overlapped I/O.
      return ERR_ASYNC;

    case ERROR_OPERATION_ABORTED:
    // case WSA_OPERATION_ABORTED: == ERROR_OPERATION_ABORTED
      ERR_LOG("Operation aborted", err);
      return ERR_ABORTED;

    case WSAEACCES:
      ERR_LOG("Access violation", err);
      return ERR_ACCESS_VIOLATION;

    case WSAEADDRNOTAVAIL:
      ERR_LOG("Address not available", err);
      return ERR_ADDRESS_NOT_AVAILABLE;

    case WSAEAFNOSUPPORT:
      ERR_LOG("Invalid option", err);
      return ERR_INVALID_OPTION;

    case WSAECONNRESET:
    case WSAEINTR:
      ERR_LOG("Connection aborted", err);
      return ERR_CONNECTION_ABORTED;

    case WSAEDESTADDRREQ:
    case WSAEFAULT:
    case WSAEINVAL:
    case WSAEMSGSIZE:
      ERR_LOG("Bad value", err);
      return ERR_INVALID_VALUE;

    case WSAENETRESET:
    case WSAEHOSTUNREACH:
    case WSAENETUNREACH:
      ERR_LOG("Network unreachable", err);
      return ERR_NETWORK_UNREACHABLE;

    case WSAENOTCONN:
    case WSAENETDOWN:
    case WSAESHUTDOWN:
      ERR_LOG("No connection", err);
      return ERR_NO_CONNECTION;

    case WSAENOTSOCK:
      ERR_LOG("Unsupported action", err);
      return ERR_UNSUPPORTED_ACTION;

    case WSAEWOULDBLOCK:
      ERR_LOG("Repeat action", err);
      return ERR_REPEAT_ACTION;

    case WSANOTINITIALISED:
      ERR_LOG("Initialization error", err);
      return ERR_INITIALIZATION;

    case WSAENOBUFS:
    case ERROR_NOT_ENOUGH_MEMORY:
    case ERROR_INVALID_USER_BUFFER:
    case ERROR_NOT_ENOUGH_QUOTA:
    case ERROR_INSUFFICIENT_BUFFER:
      ERR_LOG("OOM", err);
      return ERR_OUT_OF_MEMORY;

    case ERROR_BROKEN_PIPE:
    case ERROR_HANDLE_EOF: // XXX Maybe handle differently?
    default:
      ERR_LOG("Unexpected error", err);
      return ERR_UNEXPECTED;
  }
}



error_t
read_op(::packeteer::handle handle,
    void * dest, size_t amount, ssize_t & read,
    net::socket_address * addr)
{
  if (!handle.valid()) {
    return ERR_INVALID_VALUE;
  }

  if (amount > 0 && !dest) {
    return ERR_INVALID_VALUE;
  }

  // Initialize to error
  read = -1;

  // Copy increments refcount
  auto sys_handle = handle.sys_handle();
  const bool blocking = sys_handle->blocking;
  auto & ctx = sys_handle->read_context;

  // Check there is no other read scheduled.
  bool check_progress = false;
  if (ctx.pending_io()) {
    if (ctx.type != READ) {
      // Connect did not complete yet, try again later.
      return ERR_REPEAT_ACTION;
    }
    check_progress = true;
  }

  // Set the state, etc. to PENDING
  ctx.start_io(sys_handle->handle, READ);

  // Perform the action
  error_t err = ERR_SUCCESS;
  do {
    // Perform the action
    DWORD have_read = 0;
    BOOL res = FALSE;
    if (check_progress) {
      res = GetOverlappedResultEx(sys_handle->handle, &ctx,
          &have_read,
          blocking ? PACKETEER_EVENT_WAIT_INTERVAL_USEC / 1000 : 0,
          FALSE);
    }
    else {
      // Prepare the context
      ctx.allocate(amount);

      // Schedule read
      if (addr) {
        WSABUF buf;
        buf.buf = reinterpret_cast<CHAR *>(ctx.buf);
        buf.len = ctx.schedlen;
        DWORD flags = 0;
        INT len = ctx.address.bufsize_available();
        res = !WSARecvFrom(sys_handle->socket, &buf, 1,
            &have_read, &flags,
            reinterpret_cast<sockaddr *>(ctx.address.buffer()),
            &len,
            &ctx, nullptr);
      }
      else {
        res = ReadFile(sys_handle->handle, ctx.buf, ctx.schedlen, &have_read, &ctx);
      }

      // After the first iteration, check progress (if we iterate again).
      check_progress = true;
    }

    // Copy buffers and exit on success
    if (res) {
      // Success; copy buffer
      read = have_read;
      if (read > 0) {
        ::memcpy(dest, ctx.buf, read);
      }
      if (addr) {
        // Also store address
        *addr = ctx.address;
      }

      ctx.finish_io();
      return ERR_SUCCESS;
    }

    err = translate_error();
  } while (blocking && ERR_ASYNC == err);

  if (ERR_ASYNC == err) {
    // Operation is pending.
    return err;
  }

  ctx.finish_io();
  return translate_error();
}



error_t
write_op(::packeteer::handle handle,
    void const * source, size_t amount, ssize_t & written,
    net::socket_address const * addr)
{
  if (!handle.valid()) {
    return ERR_INVALID_VALUE;
  }

  if (!source || !amount) {
    return ERR_INVALID_VALUE;
  }

  // Initialize to error
  written = -1;

  // Copy increments refcount
  auto sys_handle = handle.sys_handle();
  const bool blocking = sys_handle->blocking;
  auto & ctx = sys_handle->write_context;

  // Check there is no other read scheduled.
  bool check_progress = false;
  if (ctx.pending_io()) {
    if (ctx.type != WRITE) {
      PACKETEER_FLOW_CONTROL_GUARD;
    }
    check_progress = true;
  }

  // Set the state
  ctx.start_io(sys_handle->handle, WRITE);

  // Perform the action
  error_t err = ERR_SUCCESS;
  do {
    DWORD have_written = 0;
    BOOL res = FALSE;
    if (check_progress) {
      res = GetOverlappedResultEx(sys_handle->handle, &ctx, &have_written,
          blocking ? PACKETEER_EVENT_WAIT_INTERVAL_USEC / 1000 : 0,
          FALSE);
    }
    else {
      // Preapre the context
      ctx.allocate(amount);
      if (amount > 0) {
        ::memcpy(ctx.buf, source, amount);
      }

      // Schedule write
      if (addr) {
        ctx.address = *addr;
        WSABUF buf;
        buf.buf = reinterpret_cast<CHAR *>(ctx.buf);
        buf.len = ctx.schedlen;
        res = !WSASendTo(sys_handle->socket, &buf, 1,
            &have_written, 0,
            reinterpret_cast<sockaddr const *>(ctx.address.buffer()),
            ctx.address.bufsize(),
            &ctx, nullptr);
      }
      else {
        res = WriteFile(sys_handle->handle, ctx.buf, ctx.schedlen, &have_written, &ctx);
      }

      // After the first iteration, check progress (if we iterate again).
      check_progress = true;
    }

    // Exit on success
    if (res) {
      // Success!
      written = have_written;

      ctx.finish_io();
      return ERR_SUCCESS;
    }

    err = translate_error();
  } while (blocking && ERR_ASYNC == err);

  ctx.finish_io();
  return err;
}


} // anonymous namespace


error_t
read(::packeteer::handle handle,
    void * buf, size_t amount, ssize_t & read)
{
  return read_op(handle, buf, amount, read, nullptr);
}



error_t
write(::packeteer::handle handle,
    void const * buf, size_t amount, ssize_t & written)
{
  return write_op(handle, buf, amount, written, nullptr);
}




error_t
receive(::packeteer::handle handle,
    void * buf, size_t amount, ssize_t & read,
    ::packeteer::net::socket_address & sender)
{
  return read_op(handle, buf, amount, read, &sender);
}



error_t
send(::packeteer::handle handle,
    void const * buf, size_t amount, ssize_t & written,
    ::packeteer::net::socket_address const & recipient)
{
  return write_op(handle, buf, amount, written, &recipient);
}



size_t
pipe_peek(::packeteer::handle handle)
{
  DWORD available = 0;
  BOOL res = PeekNamedPipe(handle.sys_handle()->handle,
      NULL, 0, NULL,
      &available,
      NULL);
  if (!res) {
    ERRNO_LOG("PeekNamedPipe failed.");
    return 0;
  }
  return available;
}



size_t
socket_peek(::packeteer::handle handle)
{
  u_long amount = 0;
  int res = ioctlsocket(handle.sys_handle()->socket,
      FIONREAD, &amount);
  if (res == SOCKET_ERROR) {
    ERRNO_LOG("ioctlsocket failed");
    return 0;
  }
  return amount;
}


} // namespace packeteer::detail::io
