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

#include "fifo.h"

#include <packeteer/handle.h>
#include <packeteer/error.h>

#include <packeteer/net/socket_address.h>

#include "fd.h"

#include "../../globals.h"
#include "../../macros.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>


namespace packeteer::detail {

namespace {

error_t
create_fifo(std::string const & path)
  OCLINT_SUPPRESS("high cyclomatic complexity")
{
  // Common to mkfifo and mknod
#if defined(PACKETEER_HAVE_MKFIFO) || defined(PACKETEER_HAVE_MKNOD)
  int ret = -1;
  int mode = S_IFIFO | S_IRUSR | S_IWUSR;
#endif

  // Availability dependent
#if defined(PACKETEER_HAVE_MKFIFO)
  ret = ::mkfifo(path.c_str(), mode);
#elif defined(PACKETEER_HAVE_MKNOD)
  ret = ::mknod(path.c_str(), mode, 0);
#else
  DLOG("platform does not support named pipes!");
  return ERR_UNEXPECTED;
#endif

  // Common to mkfifo and mknod
#if defined(PACKETEER_HAVE_MKFIFO) || defined(PACKETEER_HAVE_MKNOD)
  if (ret >= 0) {
    return ERR_SUCCESS;
  }

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

#endif
  PACKETEER_FLOW_CONTROL_GUARD;
}


error_t
translate_open_error()
  OCLINT_SUPPRESS("high cyclomatic complexity")
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

    case ENXIO:
      // ENXIO  O_NONBLOCK | O_WRONLY is set, the named file is a FIFO, and no
      //        process has the FIFO open for reading. Or, the file is a
      //        device special file and no corresponding device exists.
      return ERR_NO_CONNECTION;

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

connector_fifo::connector_fifo(std::string const & path,
    connector_options const & options)
  : connector_common(options)
  , m_addr{path}
{
}



connector_fifo::connector_fifo(net::socket_address const & addr,
    connector_options const & options)
  : connector_common(options)
  , m_addr{addr}
{
}



connector_fifo::~connector_fifo()
{
  close();
}



error_t
connector_fifo::connect()
{
  if (connected() || listening()) {
    return ERR_INITIALIZATION;
  }

  // If the file exists, open it.
  int mode = O_RDWR | O_CLOEXEC | O_ASYNC;
  if (m_options & CO_NON_BLOCKING) {
    mode |= O_NONBLOCK;
  }
  int fd = -1;
  while (true) {
    fd = ::open(m_addr.full_str().c_str(), mode);
    if (fd >= 0) {
      // All good!
      break;
    }

    ERRNO_LOG("connect() named pipe connector failed to open fifo");
    error_t err = translate_open_error();
    if (ERR_SUCCESS != err) {
      return err;
    }

    // May continue due to EINTR
  }

  m_handle = handle{fd};
  m_server = false;
  m_connected = true;

  if (m_options & CO_NON_BLOCKING) {
    return ERR_ASYNC;
  }
  return ERR_SUCCESS;
}



error_t
connector_fifo::listen()
{
  if (connected() || listening()) {
    return ERR_INITIALIZATION;
  }

  // First, create fifo
  error_t err = create_fifo(m_addr.full_str());
  if (ERR_SUCCESS != err) {
    return err;
  }

  // If the file exists, open it.
  int mode = O_RDWR | O_CLOEXEC | O_ASYNC;
  if (m_options & CO_NON_BLOCKING) {
    mode |= O_NONBLOCK;
  }
  int fd = -1;
  while (true) {
    fd = ::open(m_addr.full_str().c_str(), mode);
    if (fd >= 0) {
      // All good!
      break;
    }

    ERRNO_LOG("listen() named pipe connector failed to open fifo");
    err = translate_open_error();
    if (ERR_SUCCESS != err) {
      return err;
    }

    // May continue due to EINTR
  }

  m_handle = handle{fd};
  m_server = true;
  m_owner = true;

  return ERR_SUCCESS;
}



bool
connector_fifo::listening() const
{
  return m_handle != handle{} && m_server;
}



bool
connector_fifo::connected() const
{
  return m_handle != handle{} && m_connected;
}



connector_interface *
connector_fifo::accept(net::socket_address & addr)
{
  if (!listening()) {
    return nullptr;
  }

  // We could return this, except then we can't set the m_connected
  // flag - the server should not be connected, but the accept()ed
  // connection should.
  // In order to not return the same FD twice, we duplicate it.
  int fd = ::dup(m_handle.sys_handle());
  if (fd < 0) {
    ERRNO_LOG("Unable to dup() file handle.");
    return nullptr;
  }

  auto err = set_close_on_exit(fd);
  if (ERR_SUCCESS != err) {
    ERR_LOG("Unable to set CLOEXEC flag.", err);
    return nullptr;
  }

  // Alright, create a new connector
  auto * ret = new connector_fifo(m_addr, get_options());
  ret->m_addr = addr = m_addr;
  ret->m_server = m_server;
  ret->m_owner = false;
  ret->m_connected = true;
  ret->m_handle = handle{fd};

  return ret;
}



handle
connector_fifo::get_read_handle() const
{
  return m_handle;
}



handle
connector_fifo::get_write_handle() const
{
  return m_handle;
}



error_t
connector_fifo::close()
{
  if (!listening() && !connected()) {
    return ERR_INITIALIZATION;
  }

  // We ignore errors from close() here. This is a problem with NFS, as the man
  // pages state, but it's the price of the abstraction.
  ::close(m_handle.sys_handle());

  if (m_owner) {
    DLOG("Server closing; remove file system entry: " << m_addr.full_str());
    ::unlink(m_addr.full_str().c_str());
  }

  m_handle = handle{};
  m_server = false;
  m_owner = false;

  return ERR_SUCCESS;
}



bool
connector_fifo::is_blocking() const
{
  bool state = false;
  error_t err = detail::get_blocking_mode(m_handle.sys_handle(), state);
  if (ERR_SUCCESS != err) {
    throw exception(err, "Could not determine blocking mode from file "
        "descriptor!");
  }
  return state;
}


} // namespace packeteer::detail
