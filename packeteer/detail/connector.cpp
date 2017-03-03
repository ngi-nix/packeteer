/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2014 Unwesen Ltd.
 * Copyright (c) 2015-2017 Jens Finkhaeuser.
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

#include <packeteer/detail/connector.h>

#include <unistd.h>

namespace packeteer {
namespace detail {

connector::~connector()
{
}


// XXX how about handling multiple receive calls for different peers in one
//     iovec style call?

error_t
connector::receive(void * buf, size_t bufsize, size_t & bytes_read,
      ::packeteer::net::socket_address & sender)
{
//  ssize_t amount = ::recvfrom(get_read_handle().sys_handle(), buf, bufsize,
//
  // FIXME
//      ssize_t amount = ::recvfrom(net_fd, buf, sizeof(buf), 0,
//          static_cast<sockaddr *>(static_cast<void *>(address)), &addr_size);
// EAGAIN->EWOULDBLOCK should hand this back to caller
// EINTR same
}



error_t
connector::send(void const * buf, size_t bufsize, size_t & bytes_written,
      ::packeteer::net::socket_address const & recipient)
{
  // FIXME
//        ssize_t amount = ::sendto(net_fd,
//            envelope->serialized, envelope->serialized_size, MSG_DONTWAIT,
//            static_cast<sockaddr const *>(dest->buffer()), dest->bufsize());
// EAGAIN->EWOULDBLOCK should hand this back to caller
// EINTR same
}



size_t
connector::peek() const
{
  if (!connected() && !listening()) {
    throw exception(ERR_INITIALIZATION, "Can't peek() without listening or being connected!");
  }

  // TODO might need to simply raise or return -1 (ssize_t then) if MSG_TRUNC
  // does not exist on the target platform
  ssize_t to_read = ::recv(get_read_handle().sys_handle(), nullptr, 0,
      MSG_PEEK | MSG_TRUNC);
  if (to_read < 0) {
    switch (errno) {
      case EAGAIN:
      case EINTR:
        // Essentially ask to try again
        return 0;

      case EBADF:
      case ENOTSOCK:
        throw exception(ERR_INVALID_VALUE, errno, "Attempting to peek failed!");

      case ECONNREFUSED:
        throw exception(ERR_CONNECTION_REFUSED, errno, "Attempting to peek failed!");

      case ENOTCONN:
        throw exception(ERR_NO_CONNECTION, errno, "Attempting to peek failed!");

      case EFAULT:
        throw exception(ERR_ACCESS_VIOLATION, errno, "Attempting to peek failed!");

      case ENOMEM:
        throw exception(ERR_OUT_OF_MEMORY, errno, "Attempting to peek failed!");

      default:
        throw exception(ERR_UNEXPECTED, errno, "Attempting to peek failed!");
    }
  }
  return to_read;
}



error_t
connector::read(void * buf, size_t bufsize, size_t & bytes_read)
{
  if (!connected() && !listening()) {
    return ERR_INITIALIZATION;
  }

  while (true) {
    ssize_t read = ::read(get_read_handle().sys_handle(), buf, bufsize);
    if (-1 != read) {
      bytes_read = read;
      return ERR_SUCCESS;
    }

    bytes_read = 0;

    ERRNO_LOG("Error reading from file descriptor");
    switch (errno) {
      case EINTR: // handle signal interrupts
        continue;

      case EBADF:
      case EINVAL:
        // The file descriptor is invalid for some reason.
        return ERR_INVALID_VALUE;

      case EFAULT:
        // Technically, OOM and out of disk space/file size.
        return ERR_OUT_OF_MEMORY;

      case EIO:
      case EISDIR:
      default:
        return ERR_UNEXPECTED;
    }
  }

  PACKETEER_FLOW_CONTROL_GUARD;
}



error_t
connector::write(void const * buf, size_t bufsize, size_t & bytes_written)
{
  if (!connected() && !listening()) {
    return ERR_INITIALIZATION;
  }

  while (true) {
    ssize_t written = ::write(get_write_handle().sys_handle(), buf, bufsize);
    if (-1 != written) {
      bytes_written = written;
      return ERR_SUCCESS;
    }

    bytes_written = 0;

    ERRNO_LOG("Error writing to file descriptor");
    switch (errno) {
      case EINTR: // handle signal interrupts
        continue;

      case EBADF:
      case EINVAL:
      case EDESTADDRREQ:
      case EPIPE:
        // The file descriptor is invalid for some reason.
        return ERR_INVALID_VALUE;

      case EFAULT:
      case EFBIG:
      case ENOSPC:
        // Technically, OOM and out of disk space/file size.
        return ERR_OUT_OF_MEMORY;

      case EIO:
      default:
        return ERR_UNEXPECTED;
    }
  }

  PACKETEER_FLOW_CONTROL_GUARD;
}



}} // namespace packeteer::detail
