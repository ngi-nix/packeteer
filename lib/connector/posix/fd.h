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
#ifndef PACKETEER_CONNECTOR_POSIX_FD_H
#define PACKETEER_CONNECTOR_POSIX_FD_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#include <packeteer/handle.h>

namespace packeteer::detail {

/**
 * Set/get blocking modes on file descriptors.
 */
error_t
set_blocking_mode(handle::sys_handle_t const & fd, bool state = false);

error_t
get_blocking_mode(handle::sys_handle_t const & fd, bool & state);


/**
 * Ensure handle has O_CLOEXEC set. We always set it, so let's not complicate
 * matters.
 */
error_t
set_close_on_exit(handle::sys_handle_t const & fd);


} // namespace packeteer::detail

#endif // guard
