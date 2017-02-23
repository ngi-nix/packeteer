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



error_t
connector::receive(void * buf, size_t bufsize, size_t & bytes_read,
      ::packeteer::net::socket_address & sender)
{
  // FIXME
}



error_t
connector::send(void const * buf, size_t bufsize, size_t & bytes_written,
      ::packeteer::net::socket_address const & recipient)
{
  // FIXME
}



error_t
connector::read(void * buf, size_t bufsize, size_t & bytes_read)
{
  if (!connected()) {
    return ERR_INITIALIZATION;
  }

  ssize_t read = ::read(get_read_fd(), buf, bufsize);
  bytes_read = read;

  if (read == -1) {
    ERRNO_LOG("Error reading from file descriptor");
    switch (errno) {
      case EBADF:
      case EINVAL:
        // The file descriptor is invalid for some reason.
        return ERR_INVALID_VALUE;

      case EFAULT:
        // Technically, OOM and out of disk space/file size.
        return ERR_OUT_OF_MEMORY;

      case EINTR:
        // FIXME signal handling?
      case EIO:
      case EISDIR:
      default:
        return ERR_UNEXPECTED;
    }
  }

  return ERR_SUCCESS;
}



error_t
connector::write(void const * buf, size_t bufsize, size_t & bytes_written)
{
  if (!connected()) {
    return ERR_INITIALIZATION;
  }

  ssize_t written = ::write(get_write_fd(), buf, bufsize);
  bytes_written = written;

  if (-1 == written) {
    ERRNO_LOG("Error writing to file descriptor");
    switch (errno) {
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

      case EINTR:
        // FIXME signal handling?
      case EIO:
      default:
        return ERR_UNEXPECTED;
    }
  }

  return ERR_SUCCESS;
}



}} // namespace packeteer::detail
