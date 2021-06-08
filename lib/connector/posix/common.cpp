/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2014 Unwesen Ltd.
 * Copyright (c) 2015-2020 Jens Finkhaeuser.
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
#include <build-config.h>

#include <packeteer/connector/interface.h>

#include "common.h"

#include "../../macros.h"
#include "../../net/netincludes.h"

#include <unistd.h>
#include <sys/ioctl.h>

namespace packeteer::detail {


namespace {

inline error_t
translate_errno()
  OCLINT_SUPPRESS("high cyclomatic complexity]")
{
  switch (errno) {
    case EAGAIN: // EWOULDBLOCK
    case EINTR:
      return ERR_REPEAT_ACTION;

    case EALREADY:
      return ERR_ASYNC;

    case EBADF:
    case ENOTSOCK:
    case EINVAL:
      return ERR_INVALID_VALUE;

    case ECONNREFUSED:
      return ERR_CONNECTION_REFUSED;

    case ENOTCONN:
      return ERR_NO_CONNECTION;

    case EFAULT:
      return ERR_ACCESS_VIOLATION;

    case ENOMEM:
      return ERR_OUT_OF_MEMORY;

    case ECONNRESET:
    case EPIPE:
      return ERR_CONNECTION_ABORTED;

    case EOPNOTSUPP:
      return ERR_UNSUPPORTED_ACTION;

    default:
      return ERR_UNEXPECTED;
  }

}

} // anonymous namespace



connector_common::connector_common(peer_address const & addr,
    connector_options const & options)
  : m_options(options)
  , m_address{addr}
{
  DLOG("connector_common::connector_common(" << options << ")");
}



connector_common::~connector_common()
{
}


// XXX how about handling multiple receive calls for different peers in one
//     iovec style call?

error_t
connector_common::receive(void * buf, size_t bufsize, size_t & bytes_read,
      liberate::net::socket_address & sender)
{
  socklen_t socklen = sender.bufsize_available();
  ssize_t amount = ::recvfrom(get_read_handle().sys_handle(), buf, bufsize,
      MSG_DONTWAIT, static_cast<sockaddr *>(sender.buffer()), &socklen);

  if (amount < 0) {
    ERRNO_LOG("recvfrom failed!");
    switch (errno) {
      case EDESTADDRREQ: // Nont connection-mode socket, but no peer given.
      case EISCONN: // Connection-mode socket.
        return ERR_INVALID_OPTION;

      case EMSGSIZE: // Message size is too large
        return ERR_INVALID_VALUE;

      case ENOBUFS: // Send buffer overflow
        return ERR_NUM_ITEMS;

      default:
        return translate_errno();
    }
  }

  bytes_read = amount;
  return ERR_SUCCESS;
}



error_t
connector_common::send(void const * buf, size_t bufsize,
    size_t & bytes_written, liberate::net::socket_address const & recipient)
{
  ssize_t amount = ::sendto(get_write_handle().sys_handle(),
      buf, bufsize, MSG_DONTWAIT,
      static_cast<sockaddr const *>(recipient.buffer()), recipient.bufsize());
  if (amount < 0) {
    ERRNO_LOG("sendto failed!");
    return translate_errno();
  }

  bytes_written = amount;
  return ERR_SUCCESS;
}



size_t
connector_common::peek() const
{
  if (!connected() && !listening()) {
    throw exception(ERR_INITIALIZATION, "Can't peek() without listening or "
        "being connected!");
  }

  int bytes_available = 0;
  int err = ::ioctl(get_read_handle().sys_handle(), FIONREAD, &bytes_available);
  if (err >= 0) {
    return bytes_available;
  }

  ERRNO_LOG("ioctl failed in peek");
  switch (errno) {
    case EBADF:
      throw exception(ERR_INVALID_VALUE, errno, "Attempting to peek failed!");

    case EFAULT:
      throw exception(ERR_ACCESS_VIOLATION, errno, "Attempting to peek "
          "failed!");

    case EINVAL:
    case ENOTTY:
      throw exception(ERR_INVALID_VALUE, errno, "Attempting to peek failed!");

    default:
      throw exception(ERR_UNEXPECTED, errno, "Attempting to peek failed!");
  }
}



error_t
connector_common::read(void * buf, size_t bufsize, size_t & bytes_read)
  OCLINT_SUPPRESS("high cyclomatic complexity")
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

    if (errno == EAGAIN) {
      return ERR_ASYNC;
    }

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
connector_common::write(void const * buf, size_t bufsize,
    size_t & bytes_written)
  OCLINT_SUPPRESS("high cyclomatic complexity")
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


connector_options
connector_common::get_options() const
{
  return m_options;
}


peer_address
connector_common::peer_addr() const
{
  return m_address;
}


} // namespace packeteer::detail
