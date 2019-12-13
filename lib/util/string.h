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
 * This software is distributed on an "AS IS" BASIS, WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.
 **/
#ifndef PACKETEER_UTIL_STRING_H
#define PACKETEER_UTIL_STRING_H

#include <packeteer.h>

namespace packeteer::util {

/**
 * Lower- and uppercase strings.
 */
PACKETEER_PRIVATE
std::string to_lower(std::string const & other);

PACKETEER_PRIVATE
std::string to_upper(std::string const & other);


/**
 * Perform case-insensitive search.
 */
PACKETEER_PRIVATE
ssize_t ifind(std::string const & haystack, std::string const & needle);

#if defined(PACKETEER_WIN32)

/**
 * Convert from Windows UCS-2 to UTF-8
 */
PACKETEER_PRIVATE
std::string to_utf8(TCHAR const * source);

#endif

} // namespace packeteer::util

#endif // guard
