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
 * This software is distributed on an "AS IS" BASIS, WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.
 **/

#include <packeteer/handle.h>

#include "../macros.h"

#include <fcntl.h>
#include <unistd.h>

namespace packeteer {

namespace {

inline error_t
translate_fcntl_errno()
{
  ERRNO_LOG("fcntl error");
  switch (errno) {
    case EBADF:
    case EINVAL:
      return ERR_INVALID_VALUE;

    case EFAULT:
      return ERR_OUT_OF_MEMORY;

    case EACCES:
    case EAGAIN:
    case EINTR:
    case EBUSY:
    case EDEADLK:
    case EMFILE:
    case ENOLCK:
    case ENOTDIR:
    case EPERM:
      // TODO this is probably correct for all of these cases. check again?
      // EINTR should not occur for F_GETFL/F_SETFL; check this
      // function if it's used differently in future.
      // EACCESS/EAGAIN means the file descriptor is locked; we'll
      // ignore special treatment for the moment.
    default:
      return ERR_UNEXPECTED;
  }
}

} // anonymous namespace


error_t
set_blocking_mode(handle::sys_handle_t const & fd, bool state /* = false */)
{
  int val = ::fcntl(fd, F_GETFL, 0);
  if (-1 == val) {
    return translate_fcntl_errno();
  }

  val |= O_CLOEXEC;
  if (state) {
    val &= ~O_NONBLOCK;
  }
  else {
    val |= O_NONBLOCK;
  }
  val = ::fcntl(fd, F_SETFL, val);
  if (-1 == val) {
    ::close(fd);
    return translate_fcntl_errno();
  }

  return ERR_SUCCESS;
}


error_t
get_blocking_mode(handle::sys_handle_t const & fd, bool & state)
{
  int val = ::fcntl(fd, F_GETFL, 0);
  if (-1 == val) {
    return translate_fcntl_errno();
  }

  state = !bool(val & O_NONBLOCK);
  return ERR_SUCCESS;
}



} // namespace packeteer
