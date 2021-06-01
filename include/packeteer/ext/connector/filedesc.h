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
#ifndef PACKETEER_EXT_CONNECTOR_FILEDESC_H
#define PACKETEER_EXT_CONNECTOR_FILEDESC_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#include <packeteer/connector/types.h>

namespace packeteer::ext {

/**
 * Registers a connector type that just wraps an already opened POSIX
 * file descriptor. Use "filedesc:///123" or "fd:///123" to wrap file
 * descriptor 123.
 *
 * Special handling is added for file descriptors named "stdin",
 * "stdout" and "stderr" respectively; these are mapped to STDIN_FILENO,
 * STDOUT_FILENO and STDERR_FILENO respectively.
 *
 * Resulting connectors do not support listen() and connect() methods,
 * as the application must take care of opening them. Similarly,
 * accept() just returns the same connector.
 *
 * Also note that only CO_STREAM style connectors are supported.
 *
 * Examples:
 *   1. auto x = open(path, flags);
 *      auto conn = connector{api, "fd:///" + to_string(x)};
 *   2. auto conn = connector{api, "filedesc:///stderr"};
 */
PACKETEER_API
error_t
register_connector_filedesc(std::shared_ptr<packeteer::api> api,
    connector_type register_as = packeteer::CT_USER);

} // namespace packeteer::ext

#endif // guard
