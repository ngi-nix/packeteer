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
#ifndef PACKETEER_CONNECTOR_WIN32_IO_OPERATIONS_H
#define PACKETEER_CONNECTOR_WIN32_IO_OPERATIONS_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#include <packeteer/handle.h>

#include "overlapped.h"

namespace packeteer::detail::io {

/**
 * Functions here simulate POSIX-style I/O functions, using an
 * OverlappedManager in addition to the handle.
 *
 * The interface is different, returning an error_t and passing
 * the amount read/written/etc. as an out-parameter. But otherwise, they should
 * work very much like the POSIX equivalents.
 */
PACKETEER_PRIVATE
error_t read(
    ::packeteer::handle & handle,
    ::packeteer::detail::overlapped::manager & manager,
    void * buf, size_t amount, ssize_t & read);

PACKETEER_PRIVATE
error_t write(
    ::packeteer::handle & handle,
    ::packeteer::detail::overlapped::manager & manager,
    void const * buf, size_t amount, ssize_t & written);


} // namespace packeteer::detail::io

#endif // guard
