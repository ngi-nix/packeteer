/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2019 Jens Finkhaeuser.
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

#include "pipe_operations.h"
#include "../../util/string.h"

namespace packeteer::detail {

std::string
normalize_pipe_path(std::string const & original)
{
  if (original.empty()) {
    throw exception(ERR_INVALID_VALUE, "Cannot have empty pipe names.");
  }

  // First, replace all *unescaped* occurrences of '/' in the original string
  // with an *escaped* backslash. We're allocating twice the space of the
  // original to be on the safe side (plus '\0').
  std::vector<char> copy(original.length() * 2 + 1);
  char const * cur = original.c_str();
  char * insert = &copy[0];

  // The first character can't be escaped; at best it can be an escape sign, so let's keep it.
  *insert++ = *cur++;

  while (*cur) {
    if (*cur == '/') {
      if (*(cur - 1) != '\\') {
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

  auto copied = insert - &copy[0];
  copy.resize(copied);
  auto escaped = std::string{copy.begin(), copy.end()};

  // Now that we've escaped everything, we can case-insensitively check if the
  // required named pipe prefix is already added.
  static const std::string pipe_prefix = "\\\\.\\pipe\\";
  auto offset = ::packeteer::util::ifind(escaped, pipe_prefix);

  // If we've found the prefix *at the beginning* we're fine. Otherwise, we prepend it.
  if (0 != offset) {
    escaped = pipe_prefix + escaped;
  }

  // Now we can't find any more backslashes past the prefix.
  auto unprefixed = std::string{escaped.begin() + pipe_prefix.length(),
      escaped.end()};
  auto backslash = ::packeteer::util::ifind(unprefixed, "\\");
  if (backslash >= 0) {
    throw exception(ERR_INVALID_VALUE, "Cannot have backslashes in pipe names.");
  }

  return escaped;
}



handle
create_named_pipe(std::string const & name,
    bool blocking, bool readonly, bool remoteok /* = false */)
{
  handle::sys_handle_t res;

  std::string normalized = normalize_pipe_path(name);

  // Open mode
  DWORD open_mode = readonly ? PIPE_ACCESS_INBOUND 
                             : PIPE_ACCESS_DUPLEX;

  if (!blocking) {
    open_mode |= FILE_FLAG_OVERLAPPED;
    res.overlapped = std::make_shared<OVERLAPPED>();
  }

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
    auto err = GetLastError();
    switch (err) {
      case ERROR_INVALID_PARAMETER:
        throw exception(ERR_INVALID_OPTION, err);
	break;
      case ERROR_ACCESS_DENIED:
        throw exception(ERR_ACCESS_VIOLATION, err);
    }
    throw exception(ERR_INITIALIZATION, err);
  }

  return res;
}

} // namespace packeteer::detail
