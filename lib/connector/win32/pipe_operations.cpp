/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2019-2020 Jens Finkhaeuser.
 *
 * This software is licensed under the terms of the GNU GPLv3 for personal,
 * educational and non-profit use. For all other uses, alternative license
 * options are available. Please contact the copyright holder for additional
 * information, stating your intended usage.
 *
 * You can find the full text of the GPLv3 in the COPYING file in this code
 * distribution.
 *
 * This software is distributed on an "AS IS" BASIS, WITHOUT ANY WARRANTY
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.
 **/
#include <build-config.h>

#include <random>

#include "pipe_operations.h"
#include "overlapped.h"

#include "../../util/string.h"
#include "../../macros.h"

namespace packeteer::detail {

std::string
normalize_pipe_path(std::string const & original)
{
  if (original.empty()) {
    throw exception(ERR_INVALID_VALUE, "Cannot have empty pipe names.");
  }
  // std::cout << "Original: " << original << std::endl;

  // This is somewhat complex, because we're doing a bunch of things that can
  // interfere with each other if done differently:
  // - Preserve any \\\\.\\pipe\\ prefix, if it exists.
  // - Also preserve it if it uses slashes
  // - Then ensure that the remainder after the prefix is backslash free
  //
  // The difficulty lies in understanding which '/' to convert to '\\' (any
  // in the prefix), and which '\\' to convert to '/' (any in the name).
  //
  // We do this in two passes:
  // 1. We convert any '/' to '//' - that way, we have the string normalized
  //    enough to reason about what is part of the prefix and what isn't.
  // 2. We convert any '//' after the prefix to '/'.
  //
  // But the actual algorithm is more complex because we may or may not
  // have said prefix in the string, and want to preserve as much of the
  // original as possible.

  // Create a buffer to work on. The worst case is that all characters are
  // '/' and must be replaced by '\\', so we double the length (plus trailing
  // NUL byte).
  std::vector<char> buffer(original.length() * 2 + 1);

  // In the first pass, convert '/' to '\\' (and buffer into the buffer).
  char const * cur = original.c_str();
  char const * start = cur;
  char * insert = &buffer[0];

  while (*cur) {
    if (*cur == '/') {
      if (cur == start || *(cur - 1) != '\\') {
        // Unescaped, so replace.
        *insert++ = '\\';
        ++cur;
      }
      else {
        // Escaped, so keep - but we can strip the escape sequence.
        *(insert - 1) = '/';
        ++cur;
      }
    }
    else {
      *insert++ = *cur++;
    }
  }
  *insert = '\0'; // NUL-terminate
  size_t copylen = insert - &buffer[0];

  // std::cout << "First pass: " << std::string{&buffer[0], &buffer[0] + copylen} << std::endl;

  // Detect if we have a prefix
  static const std::string pipe_prefix = "\\\\.\\pipe\\";
  static const size_t prefix_len = pipe_prefix.length();

  auto tmp = std::string{&buffer[0], &buffer[0] + copylen};
  auto prefix_offset = ::packeteer::util::ifind(tmp, pipe_prefix);
  size_t name_offset = 0;
  if (prefix_offset == 0) {
    name_offset = prefix_len;
  }

  // std::cout << "Name offset at: " << name_offset << std::endl;

  // In the second pass, start at the name_offset, and convert any '\\' to '/'.
  // This is in the same buffer, and the size doesn't change, so we can use the
  // same pointer for reading and writing.
  insert = &buffer[0] + name_offset;

  while (*insert) {
    if (*insert == '\\') {
      *insert++ = '/';
    }
    else {
      ++insert; // No copying
    }
  }

  // std::cout << "Second pass: " << std::string{&buffer[0], &buffer[0] + copylen} << std::endl;

  // If we had a prefix, use it from the buffer. Otherwise, prepend it.
  std::string stringified{&buffer[0], &buffer[0] + copylen};
  if (name_offset) {
    return stringified;
  }
  return pipe_prefix + stringified;
}



handle
create_named_pipe(std::string const & name,
    bool blocking, bool readonly, bool remoteok /* = false */)
{
  handle::sys_handle_t res;

  std::string normalized = normalize_pipe_path(name);

  // Open mode. Always use overlapped I/O
  DWORD open_mode = readonly ? PIPE_ACCESS_INBOUND 
                             : PIPE_ACCESS_DUPLEX;
  open_mode |= FILE_FLAG_OVERLAPPED;

  // Mark handle as (non-)blocking.
  res.blocking = blocking;

  // Options
  DWORD options = PIPE_TYPE_BYTE | PIPE_WAIT | PIPE_READMODE_BYTE;

  if (remoteok) {
    options |= PIPE_ACCEPT_REMOTE_CLIENTS;
  }
  else {
    options |= PIPE_REJECT_REMOTE_CLIENTS;
  }

  // Try running create
  res.handle = CreateNamedPipeA(
      normalized.c_str(),
      open_mode,
      options,
      PIPE_UNLIMITED_INSTANCES,
      PACKETEER_IO_BUFFER_SIZE,
      PACKETEER_IO_BUFFER_SIZE,
      PACKETEER_EVENT_WAIT_INTERVAL_USEC / 1000,
      nullptr);

  if (INVALID_HANDLE_VALUE == res.handle) {
    auto err = WSAGetLastError();
    switch (err) {
      case ERROR_INVALID_PARAMETER:
        throw exception(ERR_INVALID_OPTION, err);

      case ERROR_ACCESS_DENIED:
        throw exception(ERR_ACCESS_VIOLATION, err);
    }
    throw exception(ERR_INITIALIZATION, err);
  }

  // Create the overlapped manager for the handle.
  res.overlapped_manager = std::make_shared<overlapped::manager>();

  return res;
}



error_t
poll_for_connection(handle & handle)
{
  if (!handle.valid() || !handle.sys_handle().overlapped_manager) {
    LOG("Invalid handle: need an overlapped manager");
    return ERR_INVALID_VALUE;
  }

  auto err = handle.sys_handle().overlapped_manager->schedule_overlapped(
      &(handle.sys_handle().handle), overlapped::CONNECT,
      [](overlapped::io_action action, overlapped::io_context & ctx) -> error_t
      {
        BOOL res = FALSE;

        if (overlapped::CHECK_PROGRESS == action) {
          DWORD dummy;
          res = GetOverlappedResult(*(ctx.handle), &(ctx.overlapped), &dummy,
              FALSE);
        }
        else {
          res = ConnectNamedPipe(*(ctx.handle), &(ctx.overlapped));
        }

        if (res) {
          return ERR_SUCCESS;
        }

        switch (WSAGetLastError()) {
          case ERROR_IO_PENDING:
          case ERROR_IO_INCOMPLETE: // GetOverlappedResult
            return ERR_ASYNC;

          case ERROR_PIPE_CONNECTED:
            return ERR_SUCCESS;

          default:
            ERRNO_LOG("Unexpected result of ConnectNamedPipe.");
            return ERR_CONNECTION_ABORTED;
        }
      });

  // Translate, because this function should be called again in the same way.
  if (err == ERR_ASYNC) {
    err = ERR_REPEAT_ACTION;
  }
  return err;
}



error_t
connect_to_pipe(handle & handle,
    std::string const & name, bool blocking, bool readonly)
{
  std::string normalized = normalize_pipe_path(name);

  // Handle blocking flag
  DWORD connect_flags = FILE_FLAG_OVERLAPPED;

  // Read-only
  DWORD mode = GENERIC_READ;
  DWORD share = FILE_SHARE_READ;
  if (!readonly) {
    mode |= GENERIC_WRITE;
    share |= FILE_SHARE_WRITE;
  }

  handle.sys_handle().blocking = blocking;

  // Try to connect
  HANDLE result = CreateFileA(
      normalized.c_str(),
      mode,
      share,
      nullptr, // LPSECURITY_ATTRIBUTES lpSecurityAttributes,
      OPEN_EXISTING,
      connect_flags, 
      nullptr // template handle?
  );

  // Success!
  if (result != INVALID_HANDLE_VALUE) {
    handle.sys_handle().handle = result;
    handle.sys_handle().overlapped_manager = std::make_shared<overlapped::manager>();
    return ERR_SUCCESS;
  }

  switch (WSAGetLastError()) {
    case ERROR_FILE_NOT_FOUND:
      return ERR_FS_ERROR;

    case ERROR_PIPE_BUSY:
      return ERR_REPEAT_ACTION;

    default:
      ERRNO_LOG("Unexpected result of CreateFileA.");
      return ERR_CONNECTION_ABORTED;
  }
}



std::string
create_anonymous_pipe_name(std::string const & prefix /* = "" */)
{
  // Random initializer
  std::random_device rd;
  std::uniform_int_distribution<uint32_t> dist(0); // [0, MAX)
  static volatile uint32_t serial = dist(rd);

  std::string pref = prefix;
  if (!pref.length()) {
    pref = "PacketeerAnon";
  }

  char buf[MAX_PATH];
  size_t len = ::snprintf(buf, MAX_PATH, "\\\\.\\Pipe\\%s.%08lx.%08lx",
      pref.c_str(),
      GetCurrentProcessId(),
      InterlockedIncrement(&serial)
  );

  // Normalize, because the prefix might not be.
  return normalize_pipe_path({buf, len});
}

} // namespace packeteer::detail
