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
#ifndef PACKETEER_UTIL_TMP_H
#define PACKETEER_UTIL_TMP_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#include <string>

namespace packeteer::util {

/**
 * Return a file name in a temporary path location.
 *
 * This is useful for e.g. local, pipe or fifo connectors.
 *
 * Note that the function does not guarantee the same name is not used
 * by other processes after it returns.
 *
 * Note also that on Win32, only the first three characters of the prefix
 * string are used.
 */
PACKETEER_API
std::string temp_name(std::string const & prefix = "");

} // namespace std


#endif // guard
