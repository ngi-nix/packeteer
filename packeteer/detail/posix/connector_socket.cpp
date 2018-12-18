/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2017 Jens Finkhaeuser.
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
#include <packeteer/detail/posix/connector_socket.h>

#include <packeteer/net/socket_address.h>

#include <packeteer/detail/filedescriptors.h>
#include <packeteer/detail/globals.h>

#include <packeteer/error.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>


namespace packeteer {
namespace detail {

namespace {

// FIXME 
// utility namespace for this and filedescriptors.h?
error_t
create_socket(int domain, int type, int & fd)
{
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
  error_t err = set_blocking_mode(fd, false);
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
  if (ret < 0) {
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
  }
  return ERR_SUCCESS;
}


} // anonymous namespace



connector_socket::connector_socket(net::socket_address const & addr)
  : m_addr(addr)
  , m_server(false)
  , m_fd(-1)
{
}



connector_socket::connector_socket()
  : m_addr()
  , m_server(false)
  , m_fd(-1)
{
}



error_t
connector_socket::connect(int domain, int type)
{
  if (connected() || listening()) {
    return ERR_INITIALIZATION;
  }

  // First, create socket
  int fd = -1;
  error_t err = create_socket(domain, type, fd);
  if (fd < 0) {
    return err;
  }

  // Now try to connect the socket with the path
  while (true) {
    int ret = ::connect(fd, reinterpret_cast<struct sockaddr const *>(m_addr.buffer()),
        m_addr.bufsize());
    if (ret >= 0) {
      // Finally, set the fd
      m_fd = fd;
      m_server = false;

      return ERR_SUCCESS;
    }

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
        return ERR_NUM_FILES; // technically, ports.

      case EBADF:
      case ENOTSOCK:
      case EALREADY:
      case EISCONN:
        return ERR_INITIALIZATION;

      case EINPROGRESS:
        // We'll treat this as success.
        break;

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
connector_socket::create(int domain, int type, int & fd)
{
  if (connected() || listening()) {
    return ERR_INITIALIZATION;
  }

  fd = -1;
  error_t err = create_socket(domain, type, fd);
  if (fd < 0) {
    return err;
  }
  return ERR_SUCCESS;
}



error_t
connector_socket::bind(int domain, int type, int & fd)
{
  if (connected() || listening()) {
    return ERR_INITIALIZATION;
  }

  // First, create socket
  fd = -1;
  error_t err = create_socket(domain, type, fd);
  if (fd < 0) {
    return err;
  }

  // Now try to bind the socket to the address
  int ret = ::bind(fd, reinterpret_cast<struct sockaddr const *>(m_addr.buffer()),
      m_addr.bufsize());
  if (ret < 0) {
    ::close(fd);

    ERRNO_LOG("connector_socket bind failed!");
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
        return ERR_FS_ERROR;

      case EFAULT:
      case ELOOP:
      default:
        return ERR_UNEXPECTED;
    }
  }

  return ERR_SUCCESS;
}


error_t
connector_socket::listen(int fd)
{
  if (connected() || listening()) {
    return ERR_INITIALIZATION;
  }

  // Turn the socket into a listening socket.
  int ret = ::listen(fd, PACKETEER_LISTEN_BACKLOG);
  if (ret < 0) {
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
  }
  return ERR_SUCCESS;
}




bool
connector_socket::listening() const
{
  return m_fd != -1 && m_server;
}



bool
connector_socket::connected() const
{
  return m_fd != -1 && !m_server;
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
connector_socket::close_socket()
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

  return ERR_SUCCESS;
}



error_t
connector_socket::accept(int & new_fd, net::socket_address & addr) const
{
  // There is no need for accept(); we've already got the connection established.
  if (!listening()) {
    return ERR_INITIALIZATION;
  }

  // Accept connection
  new_fd = -1;
  net::detail::address_type buf;
  ::socklen_t len = 0;

  while (true) {
    new_fd = ::accept(m_fd, &buf.sa, &len);
    if (new_fd >= 0) {
      break;
    }

    ERRNO_LOG("connector_socket accept failed!");
    switch (errno) {
      case EINTR: // signal interrupt handling
        continue;

      case EAGAIN: // EWOUlDBLOCK
        // Non-blocking server and no pending connections
        return ERR_UNEXPECTED;

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

      case ENOSR:
      case ESOCKTNOSUPPORT:
      case EPROTONOSUPPORT:
      default:
        return ERR_UNEXPECTED;
    }
  }

  // Make new socket nonblocking
  error_t err = set_blocking_mode(new_fd, false);
  if (ERR_SUCCESS != err) {
    ::close(new_fd);
    new_fd = -1;
  }
  else {
    addr = net::socket_address(&buf, len);
  }

  return err;
}


}} // namespace packeteer::detail
