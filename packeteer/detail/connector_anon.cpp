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
#include <packeteer/detail/connector_anon.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>


namespace packeteer {
namespace detail {

connector_anon::connector_anon(bool block /* = false */)
  : m_block(block)
{
  m_fds[0] = m_fds[1] = -1;
}



connector_anon::~connector_anon()
{
  close();
}



error_t
connector_anon::create_pipe()
{
  if (connected()) {
    return ERR_INITIALIZATION;
  }

  // Create pipe
  int ret = ::pipe(m_fds);
  if (-1 == ret) {
    switch (errno) {
      case EMFILE:
      case ENFILE:
        close();
        return ERR_NUM_FILES;
        break;

      default:
        close();
        return ERR_UNEXPECTED;
        break;
    }
  }

  // Optionally make the read end non-blocking
  int val = ::fcntl(m_fds[0], F_GETFL, 0);
  val |= O_CLOEXEC;
  if (!m_block) {
    val |= O_NONBLOCK;
  }
  else {
    val &= ~O_NONBLOCK;
  }
  val = ::fcntl(m_fds[0], F_SETFL, val);
  if (-1 == val) {
    // Really all errors are unexpected here.
    close();
    return ERR_UNEXPECTED;
  }

  // Optionally make the write end non-blocking
  val = ::fcntl(m_fds[1], F_GETFL, 0);
  val |= O_CLOEXEC;
  if (!m_block) {
    val |= O_NONBLOCK;
  }
  else {
    val &= ~O_NONBLOCK;
  }
  val = ::fcntl(m_fds[1], F_SETFL, val);
  if (-1 == val) {
    // Really all errors are unexpected here.
    close();
    return ERR_UNEXPECTED;
  }

  return ERR_SUCCESS;
}



error_t
connector_anon::bind()
{
  return create_pipe();
}



bool
connector_anon::bound() const
{
  return connected();
}



error_t
connector_anon::connect()
{
  return create_pipe();
}



bool
connector_anon::connected() const
{
  return (m_fds[0] != -1 && m_fds[1] != -1);
}



int
connector_anon::get_read_fd() const
{
  return m_fds[0];
}



int
connector_anon::get_write_fd() const
{
  return m_fds[1];
}



error_t
connector_anon::read(void * buf, size_t bufsize, size_t & bytes_read)
{
  if (!connected()) {
    return ERR_INITIALIZATION;
  }

  ssize_t read = ::read(m_fds[0], buf, bufsize);
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
connector_anon::write(void const * buf, size_t bufsize, size_t & bytes_written)
{
  if (!connected()) {
    return ERR_INITIALIZATION;
  }

  ssize_t written = ::write(m_fds[1], buf, bufsize);
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
connector_anon::close()
{
  if (!connected()) {
    return ERR_INITIALIZATION;
  }

  // We ignore errors from close() here. This is a problem with NFS, as the man
  // pages state, but it's the price of the abstraction.
  ::close(m_fds[0]);
  ::close(m_fds[1]);

  m_fds[0] = m_fds[1] = -1;

  return ERR_SUCCESS;
}

}} // namespace packeteer::detail
