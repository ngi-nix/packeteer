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
 * This software is distributed on an "AS IS" BASIS, WITHOUT ANY WARRANTY
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.
 **/
#include <packeteer/detail/connector_local.h>

#include <packeteer/net/socket_address.h>

#include <packeteer/detail/filedescriptors.h>
#include <packeteer/detail/globals.h>

#include <packeteer/error.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>


namespace packeteer {
namespace detail {

namespace {

int
create_socket(error_t & err)
{
  int fd = ::socket(PF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    switch (errno) {
      case EACCES:
        err = ERR_ACCESS_VIOLATION;

      case EAFNOSUPPORT:
      case EPROTONOSUPPORT:
        err = ERR_INVALID_OPTION;

      case EINVAL:
        err = ERR_INVALID_VALUE;

      case EMFILE:
      case ENFILE:
        err = ERR_NUM_FILES;

      case ENOBUFS:
      case ENOMEM:
        err = ERR_OUT_OF_MEMORY;

      default:
        err = ERR_UNEXPECTED;
    }
  }
  else {
    err = ERR_SUCCESS;
  }

  // Non-blocking
  if (ERR_SUCCESS == fd) {
    err = make_nonblocking(fd);
  }
  return fd;
}


} // anonymous namespace

connector_local::connector_local(std::string const & path)
  : m_addr(path)
  , m_server(false)
  , m_fd(-1)
{
}



connector_local::connector_local(net::socket_address const & addr)
  : m_addr(addr)
  , m_server(false)
  , m_fd(-1)
{
}



connector_local::connector_local()
  : m_addr()
  , m_server(false)
  , m_fd(-1)
{
}



connector_local::~connector_local()
{
  close();
}



error_t
connector_local::connect()
{
  if (connected() || listening()) {
    return ERR_INITIALIZATION;
  }

  // First, create socket
  error_t err = ERR_SUCCESS;
  int fd = create_socket(err);
  if (fd < 0) {
    return err;
  }

  // Now try to connect the socket with the path
  int ret = ::connect(fd, reinterpret_cast<struct sockaddr const *>(m_addr.buffer()),
      m_addr.bufsize());
  if (ret < 0) {
    ::close(fd);

    ERRNO_LOG("connector_local connect failed!");
    switch (errno) {
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

      case EINTR:
      case EFAULT:
      default:
        return ERR_UNEXPECTED;
    }
  }

  // Finally, set the fd
  m_fd = fd;
  m_server = false;

  return ERR_SUCCESS;
}



error_t
connector_local::listen()
{
  if (connected() || listening()) {
    return ERR_INITIALIZATION;
  }

  // First, create socket
  error_t err = ERR_SUCCESS;
  int fd = create_socket(err);
  if (fd < 0) {
    return err;
  }

  // Now try to bind the socket to the path
  int ret = ::bind(fd, reinterpret_cast<struct sockaddr const *>(m_addr.buffer()),
      m_addr.bufsize());
  if (ret < 0) {
    ::close(fd);

    ERRNO_LOG("connector_local bind failed!");
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

  // Turn the socket into a listening socket.
  ret = ::listen(fd, PACKETEER_LISTEN_BACKLOG);
  if (ret < 0) {
    ::close(fd);

    ERRNO_LOG("connector_local listen failed!");
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

  // Finally, set the fd
  m_fd = fd;
  m_server = true;

  return ERR_SUCCESS;
}



bool
connector_local::listening() const
{
  return m_fd != -1 && m_server;
}



bool
connector_local::connected() const
{
  return m_fd != -1 && !m_server;
}



connector *
connector_local::accept(net::socket_address & addr) const
{
  if (!m_server) {
    return nullptr;
  }

  net::detail::address_type buf;
  ::socklen_t len = 0;
  int ret = ::accept(m_fd, &buf.sa, &len);
  if (ret < 0) {
    ERRNO_LOG("connector_local accept failed!");
    switch (errno) {
      case EAGAIN: // EWOUlDBLOCK
      case EINTR:
        // Non-blocking server and no pending connections
        return nullptr;

      case EBADF:
      case EINVAL:
      case ENOTSOCK:
        throw exception(ERR_INVALID_VALUE, errno, "Server socket seems to be invalid!");

      case EOPNOTSUPP:
      case EPROTO:
        throw exception(ERR_UNSUPPORTED_ACTION, errno, "Accept not supported?");

      case ECONNABORTED:
        throw exception(ERR_CONNECTION_ABORTED, errno);

      case EFAULT:
        throw exception(ERR_ACCESS_VIOLATION, errno, "Cannot write peer address.");

      case EMFILE:
      case ENFILE:
        throw exception(ERR_NUM_FILES, errno);

      case ENOBUFS:
      case ENOMEM:
        throw exception(ERR_OUT_OF_MEMORY, errno);

      case EPERM:
        throw exception(ERR_CONNECTION_REFUSED, errno, "This might be a firewall refusing the connection.");

      case ETIMEDOUT:
        throw exception(ERR_TIMEOUT, errno);

      case ENOSR:
      case ESOCKTNOSUPPORT:
      case EPROTONOSUPPORT:
      default:
        throw exception(ERR_UNEXPECTED, errno, "Unexpected error from accept()");
    }
  }

  // Make new socket nonblocking
  error_t err = make_nonblocking(ret);
  if (ERR_SUCCESS != err) {
    ::close(ret);

    return nullptr;
  }

  // Create & return connector with accepted FD
  connector_local * result = new connector_local();
  result->m_addr = net::socket_address(&buf, len);
  result->m_server = false;
  result->m_fd = ret;

  addr = result->m_addr;
  return result;
}



int
connector_local::get_read_fd() const
{
  return m_fd;
}



int
connector_local::get_write_fd() const
{
  return m_fd;
}



error_t
connector_local::close()
{
  if (!listening() && !connected()) {
    return ERR_INITIALIZATION;
  }

  // We ignore errors from close() here. This is a problem with NFS, as the man
  // pages state, but it's the price of the abstraction.
  ::close(m_fd);
  if (m_server) {
    ::unlink(m_addr.full_str().c_str());
  }

  m_fd = -1;
  m_server = false;

  return ERR_SUCCESS;
}

}} // namespace packeteer::detail
