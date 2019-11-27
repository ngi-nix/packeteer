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
 * This software is distributed on an "AS IS" BASIS, WITHOUT ANY WARRANTY
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.
 **/
#include <packeteer/posix/detail/connector_socket.h>

#include <packeteer/net/socket_address.h>

#include <packeteer/handle.h>
#include <packeteer/detail/globals.h>

#include <packeteer/error.h>


namespace packeteer {
namespace detail {

namespace {

} // anonymous namespace



connector_socket::connector_socket(net::socket_address const & addr,
    bool blocking, connector_behaviour const & behaviour)
  : connector(blocking, behaviour)
//  , m_addr(addr)
//  , m_server(false)
//  , m_fd(-1)
{
}



connector_socket::connector_socket()
  : connector(true, CO_STREAM)
//  , m_addr()
//  , m_server(false)
//  , m_fd(-1)
{
}



error_t
connector_socket::socket_connect(int domain, int type)
{
  if (connected() || listening()) {
    return ERR_INITIALIZATION;
  }

//  // First, create socket
//  int fd = -1;
//  error_t err = create_socket(domain, type, fd, m_blocking);
//  if (fd < 0) {
//    return err;
//  }
//
//  // Now try to connect the socket with the path
//  while (true) {
//    int ret = ::connect(fd, reinterpret_cast<struct sockaddr const *>(m_addr.buffer()),
//        m_addr.bufsize());
//    if (ret >= 0) {
//      // Finally, set the fd
//      m_fd = fd;
//      m_server = false;
//
//      // Simulate non-blocking mode, also for socket types that return
//      // success. This helps the calling code treat all sockets the same.
//      if (m_blocking == false) {
//        return ERR_ASYNC;
//      }
//      return ERR_SUCCESS;
//    }
//
//    // We have a non-blocking socket, and connection will take a while to
//    // complete. We'll treat this as success, but have to return ERR_ASYNC.
//    if (errno == EINPROGRESS || errno == EALREADY) {
//      m_fd = fd;
//      m_server = false;
//      return ERR_ASYNC;
//    }
//
//    // Otherwise we have an error.
//    ::close(fd);
//
//    ERRNO_LOG("connector_socket connect failed!");
//    switch (errno) {
//      case EINTR: // handle signal interrupts
//        continue;
//
//      case EACCES:
//      case EPERM:
//        return ERR_ACCESS_VIOLATION;
//
//      case EADDRINUSE:
//        return ERR_ADDRESS_IN_USE;
//
//      case EAFNOSUPPORT:
//        return ERR_INVALID_OPTION;
//
//      case EAGAIN:
//      case EADDRNOTAVAIL:
//        return ERR_NUM_FILES; // technically, ports.
//
//      case EBADF:
//      case ENOTSOCK:
//      case EISCONN:
//        return ERR_INITIALIZATION;
//
//      case ECONNREFUSED:
//        return ERR_CONNECTION_REFUSED;
//
//      case ENETUNREACH:
//        return ERR_NETWORK_UNREACHABLE;
//
//      case ETIMEDOUT:
//        return ERR_TIMEOUT;
//
//      case EFAULT:
//      default:
//        return ERR_UNEXPECTED;
//    }
//  }

  PACKETEER_FLOW_CONTROL_GUARD;
}


error_t
connector_socket::socket_create(int domain, int type, int & fd)
{
  if (connected() || listening()) {
    return ERR_INITIALIZATION;
  }
//
//  fd = -1;
//  error_t err = create_socket(domain, type, fd, m_blocking);
//  if (fd < 0) {
//    return err;
//  }
  return ERR_SUCCESS;
}



error_t
connector_socket::socket_bind(int domain, int type, int & fd)
{
  if (connected() || listening()) {
    return ERR_INITIALIZATION;
  }

//  // First, create socket
//  fd = -1;
//  error_t err = create_socket(domain, type, fd, m_blocking);
//  if (fd < 0) {
//    return err;
//  }
//
//  // Now try to bind the socket to the address
//  int ret = ::bind(fd, reinterpret_cast<struct sockaddr const *>(m_addr.buffer()),
//      m_addr.bufsize());
//  if (ret < 0) {
//    ::close(fd);
//
//    ERRNO_LOG("connector_socket bind failed!");
//    switch (errno) {
//      case EACCES:
//        return ERR_ACCESS_VIOLATION;
//
//      case EADDRINUSE:
//        return ERR_ADDRESS_IN_USE;
//
//      case EADDRNOTAVAIL:
//        return ERR_ADDRESS_NOT_AVAILABLE;
//
//      case EAFNOSUPPORT:
//        return ERR_INVALID_OPTION;
//
//      case EAGAIN:
//        return ERR_NUM_FILES; // technically, ports.
//
//      case EINVAL:
//      case ENAMETOOLONG:
//        return ERR_INVALID_VALUE;
//
//      case EBADF:
//      case ENOTSOCK:
//        return ERR_INITIALIZATION;
//
//      case ENOMEM:
//        return ERR_OUT_OF_MEMORY;
//
//      case ENOENT:
//      case ENOTDIR:
//      case EROFS:
//        return ERR_FS_ERROR;
//
//      case EFAULT:
//      case ELOOP:
//      default:
//        return ERR_UNEXPECTED;
//    }
//  }
//
  return ERR_SUCCESS;
}


error_t
connector_socket::socket_listen(int fd)
{
  if (connected() || listening()) {
    return ERR_INITIALIZATION;
  }

//  // Turn the socket into a listening socket.
//  int ret = ::listen(fd, PACKETEER_LISTEN_BACKLOG);
//  if (ret < 0) {
//    ::close(fd);
//
//    ERRNO_LOG("connector_socket listen failed!");
//    switch (errno) {
//      case EADDRINUSE:
//        return ERR_ADDRESS_IN_USE;
//
//      case EBADF:
//      case ENOTSOCK:
//        return ERR_INVALID_VALUE;
//
//      case EOPNOTSUPP:
//        return ERR_UNSUPPORTED_ACTION;
//
//      default:
//        return ERR_UNEXPECTED;
//    }
//  }
  return ERR_SUCCESS;
}




bool
connector_socket::listening() const
{
//  return m_fd != -1 && m_server;
  return false; // FIXME
}



bool
connector_socket::connected() const
{
//  return m_fd != -1 && !m_server;
  return false; // FIXME
}



handle
connector_socket::get_read_handle() const
{
//  return handle(m_fd);
  return handle(); // FIXME
}



handle
connector_socket::get_write_handle() const
{
//  return handle(m_fd);
  return handle(); // FIXME
}



error_t
connector_socket::socket_close()
{
  if (!listening() && !connected()) {
    return ERR_INITIALIZATION;
  }

//  // We ignore errors from close() here.
//  // For local sockets, there is a problem with NFS as the man pages state, but
//  // it's the price of the abstraction.
//  ::close(m_fd);
//
//  m_fd = -1;
//  m_server = false;

  return ERR_SUCCESS;
}



error_t
connector_socket::socket_accept(int & new_fd, net::socket_address & addr) const
{
  // There is no need for accept(); we've already got the connection established.
  if (!listening()) {
    return ERR_INITIALIZATION;
  }

//  // Accept connection
//  new_fd = -1;
//  net::detail::address_type buf;
//  ::socklen_t len = 0;
//
//  while (true) {
//    new_fd = ::accept(m_fd, &buf.sa, &len);
//    if (new_fd >= 0) {
//      break;
//    }
//
//    ERRNO_LOG("connector_socket accept failed!");
//    switch (errno) {
//      case EINTR: // signal interrupt handling
//        continue;
//
//      case EAGAIN: // EWOUlDBLOCK
//        // Non-blocking server and no pending connections
//        return ERR_UNEXPECTED;
//
//      case EBADF:
//      case EINVAL:
//      case ENOTSOCK:
//        return ERR_INVALID_VALUE;
//
//      case EOPNOTSUPP:
//      case EPROTO:
//        return ERR_UNSUPPORTED_ACTION;
//
//      case ECONNABORTED:
//        return ERR_CONNECTION_ABORTED;
//
//      case EFAULT:
//        return ERR_ACCESS_VIOLATION;
//
//      case EMFILE:
//      case ENFILE:
//        return ERR_NUM_FILES;
//
//      case ENOBUFS:
//      case ENOMEM:
//        return ERR_OUT_OF_MEMORY;
//
//      case EPERM:
//        return ERR_CONNECTION_REFUSED;
//
//      case ETIMEDOUT:
//        return ERR_TIMEOUT;
//
//      // FIXME Linux only case ENOSR:
//      case ESOCKTNOSUPPORT:
//      case EPROTONOSUPPORT:
//      default:
//        return ERR_UNEXPECTED;
//    }
//  }
//
//  // Make new socket nonblocking
//  error_t err = ::packeteer::set_blocking_mode(new_fd, m_blocking);
//  if (ERR_SUCCESS != err) {
//    ::close(new_fd);
//    new_fd = -1;
//  }
//  else {
//    addr = net::socket_address(&buf, len);
//  }
  return ERR_SUCCESS;
}


error_t
connector_socket::set_blocking_mode(bool state)
{
//  return ::packeteer::set_blocking_mode(m_fd, state);
return ERR_SUCCESS; // FIXME
}



error_t
connector_socket::get_blocking_mode(bool & state) const
{
//  return ::packeteer::get_blocking_mode(m_fd, state);
return ERR_SUCCESS; // FIXME
}


}} // namespace packeteer::detail
