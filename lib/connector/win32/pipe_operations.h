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
 * This software is distributed on an "AS IS" BASIS, WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.
 **/
#ifndef PACKETEER_CONNECTOR_WIN32_PIPE_OPERATIONS_H
#define PACKETEER_CONNECTOR_WIN32_PIPE_OPERATIONS_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#include <packeteer/handle.h>

#include "../../net/netincludes.h"

namespace packeteer::detail {

/**
 * Normalize a path from a pipe URL to work for Windows. Note that this
 * function is used by create_named_pipe() internally, so there is no need to
 * pass normalized paths there.
 *
 * Path normalization preserves a pipe namespace prefix if it exists, and
 * adds it if it doesn't. It then replaces all backslashes in the unprefixed
 * names with forward slashes, mimicking UNIX path hierarchies - because pipe
 * names on Win32 cannot have hierarchy.
 **/
PACKETEER_PRIVATE
std::string normalize_pipe_path(std::string const & original);


/**
 * Create a named pipe, return a handle. The name is normalized via normalize_path() internally.
 **/
PACKETEER_PRIVATE
handle create_named_pipe(std::string const & name,
    bool blocking, bool readonly, bool remoteok = false);


/**
 * Server function for serving connections from a pipe. Requires a valid handle
 * with an overlapped_manager, as returned by create_named_pipe() on success.
 **/
PACKETEER_PRIVATE
error_t poll_for_connection(handle & handle);

/**
 * Connect to a named pipe from a client. Expected result values are:
 * - ERR_SUCCESS
 * - ERR_FS_ERROR (pipe was not created)
 * - ERR_REPEAT_ACTION (pipe was created, but we can't connect right now)
 **/
PACKETEER_PRIVATE
error_t connect_to_pipe(handle & handle, std::string const & name,
    bool blocking, bool readonly);


/**
 * Create anonymous pipe name. Optionally set the name prefix (it's not
 * necessary).
 */
PACKETEER_PRIVATE
std::string create_anonymous_pipe_name(std::string const & prefix = std::string());

} // namespace packeteer::detail

#endif // guard
