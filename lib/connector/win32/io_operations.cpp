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

#include ",,/../macros.h"
#include "../../lib/win32/sys_handle.h"

namespace packeteer::detail::io {

namespace o = ::packeteer::detail::overlapped;

error_t
read(
    ::packeteer::handle handle,
    void * buf, size_t amount, ssize_t & read)
{
  if (!handle.valid()) {
    return ERR_INVALID_VALUE;
  }

  // Initialize to error
  read = -1;

  // Copy increments refcount
  auto sys_handle = handle.sys_handle();
  const bool blocking = sys_handle->blocking;

  auto callback = [&buf, &read, blocking](o::io_action action, o::io_context & ctx) -> error_t
  {
    DWORD have_read = 0;
    BOOL res = FALSE;
    if (overlapped::CHECK_PROGRESS == action) {
      res = GetOverlappedResultEx(ctx.handle, &(ctx.overlapped), &have_read,
          blocking ? PACKETEER_EVENT_WAIT_INTERVAL_USEC / 1000 : 0,
          FALSE);
    }
    else {
      res = ReadFile(ctx.handle, ctx.buf, ctx.buflen, &have_read, &(ctx.overlapped));
    }

    if (res) {
      // Success; copy buffer
      read = have_read;
      if (read > 0) {
        ::memcpy(buf, ctx.buf, read);
      }
      return ERR_SUCCESS;
    }

    switch (WSAGetLastError()) {
      case ERROR_IO_PENDING:
      case ERROR_IO_INCOMPLETE: // GetOverlappedResultEx()
      case WAIT_TIMEOUT:
        return ERR_ASYNC;

      case ERROR_OPERATION_ABORTED:
        ERRNO_LOG("Read operationg aborted");
        return ERR_ABORTED;

      case ERROR_NOT_ENOUGH_MEMORY:
      case ERROR_INVALID_USER_BUFFER:
      case ERROR_NOT_ENOUGH_QUOTA:
      case ERROR_INSUFFICIENT_BUFFER:
        ERRNO_LOG("OOM when reading");
        return ERR_OUT_OF_MEMORY;

      case ERROR_BROKEN_PIPE:
      case ERROR_HANDLE_EOF: // XXX Maybe handle differently?
      default:
        ERRNO_LOG("Unexpected error when reading");
        return ERR_UNEXPECTED;
    }
  };


  // Loop to simulate blocking for blocking calls. This busy-loops, which is
  // not ideal.
  error_t err = ERR_UNEXPECTED;
  do {
    err = sys_handle->overlapped_manager.schedule_overlapped(
        sys_handle->handle,
        overlapped::READ,
        callback, amount);
  } while (blocking && ERR_ASYNC == err);

  if (ERR_ASYNC == err) {
    err = ERR_REPEAT_ACTION;
  }
  return err;
}



error_t
write(
    ::packeteer::handle handle,
    void const * buf, size_t amount, ssize_t & written)
{
  if (!handle.valid()) {
    return ERR_INVALID_VALUE;
  }

  // Initialize to error
  written = -1;

  // Copy increments refcount
  auto sys_handle = handle.sys_handle();
  const bool blocking = sys_handle->blocking;

  auto callback = [&buf, &written, blocking](o::io_action action, o::io_context & ctx) -> error_t
  {
    DWORD have_written = 0;
    BOOL res = FALSE;
    if (overlapped::CHECK_PROGRESS == action) {
      res = GetOverlappedResultEx(ctx.handle, &(ctx.overlapped), &have_written,
          blocking ? PACKETEER_EVENT_WAIT_INTERVAL_USEC / 1000 : 0,
          FALSE);
    }
    else {
      res = WriteFile(ctx.handle, ctx.buf, ctx.buflen, &have_written, &(ctx.overlapped));
    }

    if (res) {
      // Success!
      written = have_written;
      return ERR_SUCCESS;
    }

    switch (WSAGetLastError()) {
      case ERROR_IO_PENDING:
      case ERROR_IO_INCOMPLETE: // GetOverlappedResultEx()
      case WAIT_TIMEOUT:
        return ERR_ASYNC;

      case ERROR_OPERATION_ABORTED:
        ERRNO_LOG("Write operationg aborted");
        return ERR_ABORTED;

      case ERROR_NOT_ENOUGH_MEMORY:
      case ERROR_INVALID_USER_BUFFER:
      case ERROR_NOT_ENOUGH_QUOTA:
      case ERROR_INSUFFICIENT_BUFFER:
        ERRNO_LOG("OOM when writing");
        return ERR_OUT_OF_MEMORY;

      case ERROR_BROKEN_PIPE:
      case ERROR_HANDLE_EOF: // XXX Maybe handle differently?
      default:
        ERRNO_LOG("Unexpected error when writing");
        return ERR_UNEXPECTED;
    }
  };


  // Loop to simulate blocking for blocking calls. This busy-loops, which is
  // not ideal.
  error_t err = ERR_UNEXPECTED;
  do {
    err = sys_handle->overlapped_manager.schedule_overlapped(
        sys_handle->handle,
        overlapped::WRITE,
        callback, amount, const_cast<void *>(buf));
  } while (blocking && ERR_ASYNC == err);

  if (ERR_ASYNC == err) {
    err = ERR_REPEAT_ACTION;
  }
  return err;
}



} // namespace packeteer::detail::io
