/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@unwesen.co.uk>
 *
 * Copyright (c) 2014 Unwesen Ltd.
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




connector_local::~connector_local()
{
  close();
}



error_t
connector_local::connect()
{
  if (connected() || bound()) {
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
  std::cout << "connect returns: " << ret << std::endl;
  if (ret < 0) {
    ::close(fd);

    std::cout << "errno: " << strerror(errno) << std::endl;
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
connector_local::bind()
{
  if (connected() || bound()) {
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
  std::cout << "bind returns: " << ret << std::endl;
  if (ret < 0) {
    ::close(fd);

    std::cout << "errno: " << strerror(errno) << std::endl;
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

  // Finally, set the fd
  m_fd = fd;
  m_server = true;

  return ERR_SUCCESS;
}



bool
connector_local::bound() const
{
  return m_fd != -1 && m_server;
}



bool
connector_local::connected() const
{
  return m_fd != -1 && !m_server;
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
connector_local::read(void * buf, size_t bufsize, size_t & bytes_read)
{
  if (!bound() && !connected()) {
    return ERR_INITIALIZATION;
  }

  ssize_t read = ::read(m_fd, buf, bufsize);
  bytes_read = read;

  if (read == -1) {
    LOG("Error reading from pipe: " << ::strerror(errno));
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
connector_local::write(void const * buf, size_t bufsize, size_t & bytes_written)
{
  if (!bound() && !connected()) {
    return ERR_INITIALIZATION;
  }

  ssize_t written = ::write(m_fd, buf, bufsize);
  bytes_written = written;

  if (-1 == written) {
    LOG("Error writing to pipe: " << ::strerror(errno));
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



error_t
connector_local::close()
{
  if (!bound() && !connected()) {
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
