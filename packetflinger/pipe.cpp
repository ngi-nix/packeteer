/**
 * This file is part of packetflinger.
 *
 * Author(s): Jens Finkhaeuser <unwesen@users.sourceforge.net>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
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
#include <packetflinger/pipe.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <packetflinger/error.h>

namespace packetflinger {

pipe::pipe()
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
  val |= O_NONBLOCK | O_CLOEXEC;
  val = ::fcntl(m_fds[0], F_SETFL, val);
  if (-1 == val) {
    // Really all errors are unexpected here.
    throw exception(ERR_UNEXPECTED);
  }

  val = ::fcntl(m_fds[1], F_GETFL, 0);
  val |= O_NONBLOCK | O_CLOEXEC;
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
pipe::write(char const * buf, size_t bufsize)
{
  ssize_t written = 0;
  do {
    written = ::write(m_fds[1], buf, bufsize);
  } while (written == -1 && errno == EAGAIN);

  if (-1 == written) {
    // TODO we can do better.
    LOG("errno is: " << errno);
    return ERR_UNEXPECTED;
  }

  return ERR_SUCCESS;
}



error_t
pipe::read(char * buf, size_t bufsize, size_t & amount)
{
  ssize_t read = 0;

  do {
    read = ::read(m_fds[0], buf, bufsize);
  } while (read == -1 && errno == EAGAIN);
  amount = read;

  if (read == -1) {
    // TODO we can do better
    LOG("errno is: " << errno);
    return ERR_UNEXPECTED;
  }

  return ERR_SUCCESS;
}



int
pipe::get_read_fd()
{
  return m_fds[0];
}



int
pipe::get_write_fd()
{
  return m_fds[1];
}



} // namespace packetflinger
