/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2017-2019 Jens Finkhaeuser.
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
#ifndef PACKETEER_DETAIL_FILEDESCRIPTORS_H
#define PACKETEER_DETAIL_FILEDESCRIPTORS_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer/packeteer.h>

#include <packeteer/error.h>

namespace packeteer {
namespace detail {

/**
 * Set the blocking mode of the file descriptor (on or off).
 */
error_t
set_blocking_mode(int fd, bool blocking = false);


}} // namespace packeteer::detail

#endif // guard
