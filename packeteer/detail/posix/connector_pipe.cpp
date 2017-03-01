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
#include <packeteer/detail/posix/connector_pipe.h>

#include <packeteer/net/socket_address.h>

#include <packeteer/detail/filedescriptors.h>
#include <packeteer/detail/globals.h>

#include <packeteer/error.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>


namespace packeteer {
namespace detail {

namespace {

error_t
create_pipe(std::string const & path)
{
  // Common to mkfifo and mknod
#if defined(PACKETEER_HAVE_MKFIFO) || defined(PACKETEER_HAVE_MKNOD)
  int ret = -1;
  mode_t mode = S_IFIFO | S_IRUSR | S_IWUSR;
#endif

  // Availability dependent
#if defined(PACKETEER_HAVE_MKFIFO)
  ret = ::mkfifo(path.c_str(), mode);
#elif defined(PACKETEER_HAVE_MKNOD)
  ret = ::mknod(path.c_str(), mode, 0);
#else
  LOG("platform does not support named pipes!");
  return ERR_UNEXPECTED;
#endif

  // Common to mkfifo and mknod
#if defined(PACKETEER_HAVE_MKFIFO) || defined(PACKETEER_HAVE_MKNOD)
  if (ret < 0) {
    ERRNO_LOG("Creating named pipe failed!");
    switch (errno) {
      case EACCES:
      case EFAULT:
        return ERR_ACCESS_VIOLATION;

      case EDQUOT:
      case ELOOP:
      case ENOENT:
      case ENOSPC:
      case ENOTDIR:
      case EROFS:
        return ERR_FS_ERROR;

      case EEXIST:
        // Can't do anything but try to use this existing file as a pipe
        break;

      case ENAMETOOLONG:
        return ERR_INVALID_OPTION;

      case ENOMEM:
        return ERR_OUT_OF_MEMORY;

      case EPERM:
      case EINVAL:
      case EBADF:
      default:
        return ERR_UNEXPECTED;
    }
  }

  // Pipe was either created here, or file exists.
#endif
  return ERR_SUCCESS;
}


error_t
translate_open_error()
{
  switch (errno) {
    case EINTR: // signal interrupt handling
      return ERR_SUCCESS; // contract with caller

    case EACCES:
    case EFAULT:
      return ERR_ACCESS_VIOLATION;

    case EDQUOT:
    case EEXIST:
    case EFBIG:
    case EISDIR:
    case ELOOP:
    case ENOENT:
    case ENOSPC:
    case ENOTDIR:
    case EROFS:
    case ENAMETOOLONG:
    case EWOULDBLOCK:
      return ERR_FS_ERROR;

    case EINVAL:

    case EMFILE:
    case ENFILE:
      return ERR_NUM_FILES;


    case ENOMEM:
    case EOVERFLOW:
      return ERR_OUT_OF_MEMORY;

    // FIXME
    // ENXIO  O_NONBLOCK | O_WRONLY is set, the named file is a FIFO, and no process has the  FIFO
    //        open for reading.  Or, the file is a device special file and no corresponding device
    //        exists.

    case ENXIO:
    case EOPNOTSUPP:
      return ERR_UNSUPPORTED_ACTION;

    case ENODEV:
    case EPERM:
    case ETXTBSY:
    default:
      return ERR_UNEXPECTED;
  }
}

} // anonymous namespace

connector_pipe::connector_pipe(std::string const & path)
  : m_addr(path)
  , m_server(false)
  , m_fd(-1)
{
}



connector_pipe::connector_pipe(net::socket_address const & addr)
  : m_addr(addr)
  , m_server(false)
  , m_fd(-1)
{
}



connector_pipe::~connector_pipe()
{
  close();
}



error_t
connector_pipe::connect()
{
  if (connected() || listening()) {
    return ERR_INITIALIZATION;
  }

  // If the file exists, open it.
  mode_t mode = O_RDWR | O_CLOEXEC | O_ASYNC | O_NONBLOCK;
  int fd = -1;
  while (true) {
    fd = ::open(m_addr.full_str().c_str(), mode);
    if (fd >= 0) {
      // All good!
      break;
    }

    ERRNO_LOG("connect() named pipe connector failed to open pipe");
    error_t err = translate_open_error();
    if (ERR_SUCCESS == err) {
      // signal interrupt handling
      continue;
    }
    return err;
  }

  m_fd = fd;
  m_server = false;

  return ERR_SUCCESS;
}



error_t
connector_pipe::listen()
{
  if (connected() || listening()) {
    return ERR_INITIALIZATION;
  }

  // First, create pipe
  error_t err = create_pipe(m_addr.full_str());
  if (ERR_SUCCESS != err) {
    return err;
  }

  // If the file exists, open it.
  mode_t mode = O_RDWR | O_CLOEXEC | O_ASYNC | O_NONBLOCK;
  int fd = -1;
  while (true) {
    fd = ::open(m_addr.full_str().c_str(), mode);
    if (fd >= 0) {
      // All good!
      break;
    }

    ERRNO_LOG("listen() named pipe connector failed to open pipe");
    error_t err = translate_open_error();
    if (ERR_SUCCESS == err) {
      // signal interrupt handling
      continue;
    }
    return err;
  }

  m_fd = fd;
  m_server = true;

  return ERR_SUCCESS;
}



bool
connector_pipe::listening() const
{
  return m_fd != -1 && m_server;
}



bool
connector_pipe::connected() const
{
  return m_fd != -1 && !m_server;
}



connector *
connector_pipe::accept(net::socket_address & /* unused */) const
{
  // There is no need for accept(); we've already got the connection established.
  if (!listening()) {
    return nullptr;
  }
  return const_cast<connector_pipe*>(this);
}



handle
connector_pipe::get_read_handle() const
{
  return handle(m_fd);
}



handle
connector_pipe::get_write_handle() const
{
  return handle(m_fd);
}



error_t
connector_pipe::close()
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
