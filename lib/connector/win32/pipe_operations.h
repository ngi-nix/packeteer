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
 * Path normalization replaces all *unquoted* forward slashes '/' with
 * backslashes '\' - escaped so that they can be passed to Win32 APIs.
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
 * Server function for serving connections from a pipe. Requires a non-blocking
 * handle, and will return ERR_UNSUPPORTED_ACTION when a blocking handle is
 * passed.
 **/
PACKETEER_PRIVATE
error_t poll_for_connection(handle const & handle);

/**
 * Connect to a named pipe from a client. Expected result values are:
 * - ERR_SUCCESS
 * - ERR_FS_ERROR (pipe was not created)
 * - ERR_REPEAT_ACTION (pipe was created, but we can't connect right now)
 **/
PACKETEER_PRIVATE
error_t connect_to_pipe(handle & handle, std::string const & name, bool blocking,
    bool readonly);

// TODO:
// - Helper for anonymous pipes (see stackexchange)

} // namespace packeteer::detail

#endif // guard
