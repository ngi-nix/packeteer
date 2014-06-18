/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@unwesen.co.uk>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 * Copyright (c) 2012-2014 Unwesen Ltd.
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
#include <packeteer/pipe.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <packeteer/error.h>

namespace packeteer {

pipe::pipe(bool block /* = true */)
  : m_fds({ -1, -1})
{
  int ret = ::pipe(m_fds);
  if (-1 == ret) {
    switch (errno) {
      case EMFILE:
      case ENFILE:
        throw exception(ERR_NUM_FILES);
        break;

      default:
        throw exception(ERR_UNEXPECTED);
        break;
    }
  }

  // Make the read end non-blocking.
  int val = ::fcntl(m_fds[0], F_GETFL, 0);
  val |= O_CLOEXEC;
  if (!block) {
    val |= O_NONBLOCK;
  }
  else {
    val &= ~O_NONBLOCK;
  }
  val = ::fcntl(m_fds[0], F_SETFL, val);
  if (-1 == val) {
    // Really all errors are unexpected here.
    throw exception(ERR_UNEXPECTED);
  }

  // Make the write end non-blocking
  val = ::fcntl(m_fds[1], F_GETFL, 0);
  val |= O_CLOEXEC;
  if (!block) {
    val |= O_NONBLOCK;
  }
  else {
    val &= ~O_NONBLOCK;
  }
  val = ::fcntl(m_fds[1], F_SETFL, val);
  if (-1 == val) {
    throw exception(ERR_UNEXPECTED);
  }
}



pipe::~pipe()
{
  ::close(m_fds[0]);
  ::close(m_fds[1]);
}



error_t
pipe::write(char const * buf, size_t bufsize, size_t & bytes_written)
{
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
pipe::read(char * buf, size_t bufsize, size_t & bytes_read)
{
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



int
pipe::get_read_fd() const
{
  return m_fds[0];
}



int
pipe::get_write_fd() const
{
  return m_fds[1];
}



} // namespace packeteer
