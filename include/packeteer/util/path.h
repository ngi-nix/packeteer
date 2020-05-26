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
#ifndef PACKETEER_UTIL_PATH_H
#define PACKETEER_UTIL_PATH_H

#include <packeteer.h>

namespace packeteer::util {

/**
 * Convert path names from POSIX-like to WIN32 and back.
 *
 * This mostly converts any unquoted backslash unto forward slash and vice
 * versa. But it also converts `<drivename>:\` to `/<drivename>/` and back.
 *
 * In the end, it's this functionality that allows you to use POSIX-style
 * path segments in URLs, and have them map to Windows paths on WIN32.
 */
PACKETEER_API
std::string to_posix_path(std::string const & other);

PACKETEER_API
std::string to_win32_path(std::string const & other);

} // namespace packeteer::util

#endif // guard
