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
#include <build-config.h>

#include <packeteer/connector/interface.h>

#include "io_operations.h"
#include "../../macros.h"

namespace packeteer {

connector_interface::connector_interface(connector_options const & options)
  : m_options(options)
{
  LOG("connector_interface::connector_interface(" << options << ")");
}



connector_interface::~connector_interface()
{
}


error_t
connector_interface::receive(void * buf, size_t bufsize, size_t & bytes_read,
      ::packeteer::net::socket_address & sender)
{
#if 0
  socklen_t socklen = sender.bufsize_available();
  ssize_t amount = ::recvfrom(get_read_handle().sys_handle(), buf, bufsize,
      MSG_DONTWAIT, static_cast<sockaddr *>(sender.buffer()), &socklen);

  if (amount < 0) {
    ERRNO_LOG("recvfrom failed!");
    switch (errno) {
      case EAGAIN: // EWOULDBLOCK
      case EINTR:
        return ERR_REPEAT_ACTION;

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

      default:
        return ERR_UNEXPECTED;
    }
  }

  bytes_read = amount;
#endif
  return ERR_SUCCESS;
}



error_t
connector_interface::send(void const * buf, size_t bufsize, size_t & bytes_written,
      ::packeteer::net::socket_address const & recipient)
{
#if 0
  ssize_t amount = ::sendto(get_write_handle().sys_handle(),
      buf, bufsize, MSG_DONTWAIT,
      static_cast<sockaddr const *>(recipient.buffer()), recipient.bufsize());
  if (amount < 0) {
    ERRNO_LOG("sendto failed!");
    switch (errno) {
      case EAGAIN: // EWOULDBLOCK
      case EINTR:
        return ERR_REPEAT_ACTION;

      case EALREADY:
        return ERR_ASYNC;

      case EDESTADDRREQ: // Nont connection-mode socket, but no peer given.
      case EISCONN: // Connection-mode socket.
        return ERR_INVALID_OPTION;

      case EMSGSIZE: // Message size is too large
        return ERR_INVALID_VALUE;

      case ENOBUFS: // Send buffer overflow
        return ERR_NUM_ITEMS;

      case EBADF:
      case ENOTSOCK:
      case EINVAL:
        return ERR_INVALID_VALUE;

      case ECONNREFUSED:
        return ERR_CONNECTION_REFUSED;

      case ECONNRESET:
      case EPIPE:
        return ERR_CONNECTION_ABORTED;

      case ENOTCONN:
        return ERR_NO_CONNECTION;

      case EFAULT:
        return ERR_ACCESS_VIOLATION;

      case ENOMEM:
        return ERR_OUT_OF_MEMORY;

      case EOPNOTSUPP:
        return ERR_UNSUPPORTED_ACTION;

      default:
        return ERR_UNEXPECTED;
    }
  }

  bytes_written = amount;
#endif
  return ERR_SUCCESS;
}



size_t
connector_interface::peek() const
{
#if 0
  if (!connected() && !listening()) {
    throw exception(ERR_INITIALIZATION, "Can't peek() without listening or being connected!");
  }

  // TODO might need to simply raise or return -1 (ssize_t then) if MSG_TRUNC
  // does not exist on the target platform
  ssize_t to_read = ::recv(get_read_handle().sys_handle(), nullptr, 0,
      MSG_PEEK | MSG_TRUNC);
  if (to_read < 0) {
    ERRNO_LOG("recv failed in peek");
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
#endif
  return 0; // FIXME
}



error_t
connector_interface::read(void * buf, size_t bufsize, size_t & bytes_read)
{
  if (!connected() && !listening()) {
    return ERR_INITIALIZATION;
  }

  ssize_t read = -1;
  auto err = detail::io::read(get_read_handle(), buf, bufsize, read);
  if (ERR_SUCCESS == err) {
    bytes_read = read;
  }
  return err;
}



error_t
connector_interface::write(void const * buf, size_t bufsize, size_t & bytes_written)
{
  if (!connected() && !listening()) {
    return ERR_INITIALIZATION;
  }

  ssize_t written = -1;
  auto err = detail::io::write(get_write_handle(), buf, bufsize, written);
  if (ERR_SUCCESS == err) {
    bytes_written = written;
  }
  return err;
}


connector_options
connector_interface::get_options() const
{
  return m_options;
}


} // namespace packeteer
