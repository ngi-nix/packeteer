/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2017-2020 Jens Finkhaeuser.
 *
 * This software is licensed under the terms of the GNU GPLv3 for personal,
 * educational and non-profit use. For all other uses, alternative license
 * options are available. Please contact the copyright holder for additional
 * information, stating your intended usage.
 *
 * You can find the full text of the GPLv3 in the COPYING file in this code
 * distribution.
 *
 * This software is distributed on an "AS IS" BASIS, WITHOUT ANY WARRANTY
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.
 **/
#include <build-config.h>

#include "socket.h"

#include <packeteer/handle.h>
#include <packeteer/error.h>

#include "fd.h"
#include "../../globals.h"
#include "../../macros.h"

#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>


namespace packeteer::detail {

namespace {

error_t
create_socket(int domain, int type, int & fd, bool blocking)
  OCLINT_SUPPRESS("high ncss method")
  OCLINT_SUPPRESS("high cyclomatic complexity")
{
  DLOG("create_socket(" << blocking << ")");
  fd = ::socket(domain, type, 0);
  if (fd < 0) {
    ERRNO_LOG("create_socket socket failed!");
    switch (errno) {
      case EACCES:
        return ERR_ACCESS_VIOLATION;

      case EAFNOSUPPORT:
      case EPROTONOSUPPORT:
        return ERR_INVALID_OPTION;

      case EINVAL:
        return ERR_INVALID_VALUE;

      case EMFILE:
      case ENFILE:
        return ERR_NUM_FILES;

      case ENOBUFS:
      case ENOMEM:
        return ERR_OUT_OF_MEMORY;

      default:
        return ERR_UNEXPECTED;
    }
  }

  // Non-blocking
  error_t err = detail::set_blocking_mode(fd, blocking);
  if (ERR_SUCCESS != err) {
    ::close(fd);
    fd = -1;
    return err;
  }

  // Set socket to close forcibly.
  ::linger option;
  option.l_onoff = 1;
  option.l_linger = 0;
  int ret = ::setsockopt(fd, SOL_SOCKET, SO_LINGER, &option, sizeof(option));
  if (ret >= 0) {
    return ERR_SUCCESS;
  }

  ::close(fd);
  fd = -1;

  ERRNO_LOG("create_socket setsockopt failed!");
  switch (errno) {
    case EBADF:
    case EFAULT:
    case EINVAL:
      return ERR_INVALID_VALUE;

    case ENOPROTOOPT:
    case ENOTSOCK:
      return ERR_UNSUPPORTED_ACTION;

    default:
      return ERR_UNEXPECTED;
  }

  PACKETEER_FLOW_CONTROL_GUARD;
}


} // anonymous namespace



connector_socket::connector_socket(liberate::net::socket_address const & addr,
    connector_options const & options)
  : connector_common(options)
  , m_addr(addr)
{
}



connector_socket::connector_socket(connector_options const & options)
  : connector_common(options)
{
}



error_t
connector_socket::socket_connect(int domain, int type)
  OCLINT_SUPPRESS("high ncss method")
  OCLINT_SUPPRESS("high npath complexity")
  OCLINT_SUPPRESS("high cyclomatic complexity")
  OCLINT_SUPPRESS("long method")
{
  if (connected() || listening()) {
    return ERR_INITIALIZATION;
  }

  // https://gitlab.com/interpeer/packeteer/-/issues/18
  if (m_addr.type() == liberate::net::AT_UNSPEC) {
    ELOG("Unnamed CT_LOCAL connectors are not supported yet.");
    return ERR_INVALID_VALUE;
  }

  // First, create socket
  int fd = -1;
  error_t err = create_socket(domain, type, fd, m_options & CO_BLOCKING);
  if (fd < 0) {
    return err;
  }

  // Now try to connect the socket with the path
  while (true) {
    int ret = ::connect(fd,
        reinterpret_cast<struct sockaddr const *>(m_addr.buffer()),
        m_addr.bufsize());
    if (ret >= 0) {
      // Finally, set the fd
      m_fd = fd;
      m_server = false;
      m_connected = true;

      // Simulate non-blocking mode, also for socket types that return
      // success. This helps the calling code treat all sockets the same.
      if (m_options & CO_NON_BLOCKING) {
        return ERR_ASYNC;
      }
      return ERR_SUCCESS;
    }

    // We have a non-blocking socket, and connection will take a while to
    // complete. We'll treat this as success, but have to return ERR_ASYNC.
    if (errno == EINPROGRESS || errno == EALREADY) {
      m_fd = fd;
      m_server = false;
      m_connected = true;
      return ERR_ASYNC;
    }

    // Otherwise we have an error.
    ::close(fd);

    ERRNO_LOG("connector_socket connect failed!");
    switch (errno) {
      case EINTR: // handle signal interrupts
        continue;

      case EACCES:
      case EPERM:
        return ERR_ACCESS_VIOLATION;

      case EADDRINUSE:
        return ERR_ADDRESS_IN_USE;

      case EAFNOSUPPORT:
        return ERR_INVALID_OPTION;

      case EAGAIN:
      case EADDRNOTAVAIL:
        return ERR_NUM_FILES; // technically, ports.

      case EBADF:
      case ENOTSOCK:
      case EISCONN:
        return ERR_INITIALIZATION;

      case ECONNREFUSED:
        return ERR_CONNECTION_REFUSED;

      case ENETUNREACH:
        return ERR_NETWORK_UNREACHABLE;

      case ETIMEDOUT:
        return ERR_TIMEOUT;

      case EFAULT:
      default:
        return ERR_UNEXPECTED;
    }
  }

  PACKETEER_FLOW_CONTROL_GUARD;
}


error_t
connector_socket::socket_create(int domain, int type, int & fd)
{
  if (connected() || listening()) {
    return ERR_INITIALIZATION;
  }

  fd = -1;
  error_t err = create_socket(domain, type, fd, m_options & CO_BLOCKING);
  if (fd < 0) {
    return err;
  }
  return ERR_SUCCESS;
}



error_t
connector_socket::socket_bind(int domain, int type, int & fd)
  OCLINT_SUPPRESS("high npath complexity")
  OCLINT_SUPPRESS("high cyclomatic complexity")
{
  if (connected() || listening()) {
    return ERR_INITIALIZATION;
  }

  // https://gitlab.com/interpeer/packeteer/-/issues/18
  if (m_addr.type() == liberate::net::AT_UNSPEC) {
    ELOG("Unnamed CT_LOCAL connectors are not supported yet.");
    return ERR_INVALID_VALUE;
  }

  // First, create socket
  fd = -1;
  error_t err = create_socket(domain, type, fd, m_options & CO_BLOCKING);
  if (fd < 0) {
    return err;
  }

  // Now try to bind the socket to the address
  int ret = ::bind(fd,
      reinterpret_cast<struct sockaddr const *>(m_addr.buffer()),
      m_addr.bufsize());
  if (ret >= 0) {
    return ERR_SUCCESS;
  }

  ::close(fd);

  ERRNO_LOG("connector_socket bind failed; address is: " << m_addr.full_str());
  switch (errno) {
    case EACCES:
      return ERR_ACCESS_VIOLATION;

    case EADDRINUSE:
      return ERR_ADDRESS_IN_USE;

    case EADDRNOTAVAIL:
      return ERR_ADDRESS_NOT_AVAILABLE;

    case EAFNOSUPPORT:
      return ERR_INVALID_OPTION;

    case EAGAIN:
      return ERR_NUM_FILES; // technically, ports.

    case EINVAL:
    case ENAMETOOLONG:
      return ERR_INVALID_VALUE;

    case EBADF:
    case ENOTSOCK:
      return ERR_INITIALIZATION;

    case ENOMEM:
      return ERR_OUT_OF_MEMORY;

    case ENOENT:
    case ENOTDIR:
    case EROFS:
      // If this is due to an abstract address, return a different error.
      if (m_addr.full_str()[0] == '\0') {
        return ERR_INVALID_OPTION;
      }
      return ERR_FS_ERROR;

    case EFAULT:
    case ELOOP:
    default:
      return ERR_UNEXPECTED;
  }

  PACKETEER_FLOW_CONTROL_GUARD;
}


error_t
connector_socket::socket_listen(int fd)
{
  if (connected() || listening()) {
    return ERR_INITIALIZATION;
  }

  // Turn the socket into a listening socket.
  int ret = ::listen(fd, PACKETEER_LISTEN_BACKLOG);
  if (ret >= 0) {
    return ERR_SUCCESS;
  }

  ::close(fd);

  ERRNO_LOG("connector_socket listen failed!");
  switch (errno) {
    case EADDRINUSE:
      return ERR_ADDRESS_IN_USE;

    case EBADF:
    case ENOTSOCK:
      return ERR_INVALID_VALUE;

    case EOPNOTSUPP:
      return ERR_UNSUPPORTED_ACTION;

    default:
      return ERR_UNEXPECTED;
  }

  PACKETEER_FLOW_CONTROL_GUARD;
}




bool
connector_socket::listening() const
{
  return m_fd != -1 && m_server;
}



bool
connector_socket::connected() const
{
  return m_fd != -1 && m_connected;
}



handle
connector_socket::get_read_handle() const
{
  return handle(m_fd);
}



handle
connector_socket::get_write_handle() const
{
  return handle(m_fd);
}



error_t
connector_socket::socket_close()
{
  if (!listening() && !connected()) {
    return ERR_INITIALIZATION;
  }

  // We ignore errors from close() here.
  // For local sockets, there is a problem with NFS as the man pages state, but
  // it's the price of the abstraction.
  ::close(m_fd);

  m_fd = -1;
  m_server = false;
  m_connected = false;

  return ERR_SUCCESS;
}



error_t
connector_socket::socket_accept(int & new_fd, liberate::net::socket_address & addr)
  OCLINT_SUPPRESS("high cyclomatic complexity")
  OCLINT_SUPPRESS("long method")
{
  // There is no need for accept(); we've already got the connection
  // established.
  if (!listening()) {
    return ERR_INITIALIZATION;
  }

  // Accept connection
  new_fd = -1;
  liberate::net::detail::address_data buf;
  ::socklen_t len = sizeof(buf);

  while (true) {
    new_fd = ::accept(m_fd, reinterpret_cast<sockaddr *>(&buf), &len);
    if (new_fd >= 0) {
      break;
    }

    ERRNO_LOG("connector_socket accept failed!");
    switch (errno) {
      case EINTR: // signal interrupt handling
        continue;

      case EAGAIN: // EWOULDBLOCK
        // This is not an error; it just means there is no
        // pending connection on a non-blocking connector.
        // But epoll() etc. still claim the server socket is
        // readable, which is not convenient.
        return ERR_REPEAT_ACTION;

      case EBADF:
      case EINVAL:
      case ENOTSOCK:
        return ERR_INVALID_VALUE;

      case EOPNOTSUPP:
      case EPROTO:
        return ERR_UNSUPPORTED_ACTION;

      case ECONNABORTED:
        return ERR_CONNECTION_ABORTED;

      case EFAULT:
        return ERR_ACCESS_VIOLATION;

      case EMFILE:
      case ENFILE:
        return ERR_NUM_FILES;

      case ENOBUFS:
      case ENOMEM:
        return ERR_OUT_OF_MEMORY;

      case EPERM:
        return ERR_CONNECTION_REFUSED;

      case ETIMEDOUT:
        return ERR_TIMEOUT;

      case ESOCKTNOSUPPORT:
      case EPROTONOSUPPORT:
      // Linux only case ENOSR:
      default:
        return ERR_UNEXPECTED;
    }
  }

  // Make new socket nonblocking
  error_t err = detail::set_blocking_mode(new_fd, m_options & CO_BLOCKING);
  if (ERR_SUCCESS != err) {
    ::close(new_fd);
    new_fd = -1;
    return err;
  }

  // Keep address and return success.
  addr = liberate::net::socket_address(&buf, len);
  return ERR_SUCCESS;
}



bool
connector_socket::is_blocking() const
{
  bool state = false;
  error_t err = detail::get_blocking_mode(m_fd, state);
  if (ERR_SUCCESS != err) {
    throw exception(err, "Could not determine blocking mode from file "
        "descriptor!");
  }
  return state;
}


} // namespace packeteer::detail
