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

#include <liberate/net/socket_address.h>

#include <packeteer/handle.h>


namespace packeteer::detail {

/**
 * Functions here simulate POSIX-style I/O functions, using an
 * overlapped contexts in the handle.
 *
 * The interface is different, returning an error_t and passing
 * the amount read/written/etc. as an out-parameter. But otherwise, they should
 * work very much like the POSIX equivalents.
 */
PACKETEER_PRIVATE
error_t read(
    ::packeteer::handle handle,
    void * buf, size_t amount, ssize_t & read);

PACKETEER_PRIVATE
error_t write(
    ::packeteer::handle handle,
    void const * buf, size_t amount, ssize_t & written);


PACKETEER_PRIVATE
error_t receive(
    ::packeteer::handle handle,
    void * buf, size_t amount, ssize_t & read,
    ::liberate::net::socket_address & sender);

PACKETEER_PRIVATE
error_t send(
    ::packeteer::handle handle,
    void const * buf, size_t amount, ssize_t & written,
    ::liberate::net::socket_address const & recipient);



/**
 * Peek a named pipe handle or socket. Uses the simple peek semantics of our
 * connectors where no actual data is returned.
 */
PACKETEER_PRIVATE
size_t pipe_peek(::packeteer::handle handle);

PACKETEER_PRIVATE
size_t socket_peek(::packeteer::handle handle);


/**
 * The zero-byte read is a special opreation that should not be used by
 * connectors. It is of use to the I/O subsystem implementation.
 */
PACKETEER_PRIVATE
error_t zero_byte_read(::packeteer::handle handle);

} // namespace packeteer::detail

#endif // guard
