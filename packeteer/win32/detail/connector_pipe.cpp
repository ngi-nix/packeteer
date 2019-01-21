/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2019 Jens Finkhaeuser.
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
#include <packeteer/detail/connector_pipe.h>

#include <packeteer/handle.h>
#include <packeteer/error.h>

#include <packeteer/net/socket_address.h>

#include <packeteer/detail/globals.h>


namespace packeteer {
namespace detail {

namespace {

} // anonymous namespace

connector_pipe::connector_pipe(std::string const & path, bool blocking)
//  : connector(blocking, CB_STREAM)
//  , m_addr(path)
//  , m_server(false)
//  , m_fd(-1)
{
}



connector_pipe::connector_pipe(net::socket_address const & addr, bool blocking)
//  : connector(blocking, CB_STREAM)
//  , m_addr(addr)
//  , m_server(false)
//  , m_fd(-1)
{
}



connector_pipe::~connector_pipe()
{
  close();
}



error_t
connector_pipe::connect()
{
//  if (connected() || listening()) {
//    return ERR_INITIALIZATION;
//  }
//
//  // If the file exists, open it.
//  int mode = O_RDWR | O_CLOEXEC | O_ASYNC;
//  if (!m_blocking) {
//    mode |= O_NONBLOCK;
//  }
//  int fd = -1;
//  while (true) {
//    fd = ::open(m_addr.full_str().c_str(), mode);
//    if (fd >= 0) {
//      // All good!
//      break;
//    }
//
//    ERRNO_LOG("connect() named pipe connector failed to open pipe");
//    error_t err = translate_open_error();
//    if (ERR_SUCCESS == err) {
//      // signal interrupt handling
//      continue;
//    }
//    return err;
//  }
//
//  m_fd = fd;
//  m_server = false;
//
//  if (!m_blocking) {
//    return ERR_ASYNC;
//  }
  return ERR_SUCCESS;
}



error_t
connector_pipe::listen()
{
  if (connected() || listening()) {
    return ERR_INITIALIZATION;
  }

//  // First, create pipe
//  error_t err = create_pipe(m_addr.full_str());
//  if (ERR_SUCCESS != err) {
//    return err;
//  }
//
//  // If the file exists, open it.
//  int mode = O_RDWR | O_CLOEXEC | O_ASYNC;
//  if (!m_blocking) {
//    mode |= O_NONBLOCK;
//  }
//  int fd = -1;
//  while (true) {
//    fd = ::open(m_addr.full_str().c_str(), mode);
//    if (fd >= 0) {
//      // All good!
//      break;
//    }
//
//    ERRNO_LOG("listen() named pipe connector failed to open pipe");
//    error_t err = translate_open_error();
//    if (ERR_SUCCESS == err) {
//      // signal interrupt handling
//      continue;
//    }
//    return err;
//  }
//
//  m_fd = fd;
//  m_server = true;

  return ERR_SUCCESS;
}



bool
connector_pipe::listening() const
{
//  return m_fd != -1 && m_server;
	return false; // FIXME
}



bool
connector_pipe::connected() const
{
//  return m_fd != -1 && !m_server;
	return false; // FIXME
}



connector *
connector_pipe::accept(net::socket_address & /* unused */) const
{
  // There is no need for accept(); we've already got the connection established.
  if (!listening()) {
    return nullptr;
  }
  return const_cast<connector_pipe *>(this);
}



handle
connector_pipe::get_read_handle() const
{
//  return handle(m_fd);
	return handle(); // FIXME
}



handle
connector_pipe::get_write_handle() const
{
//  return handle(m_fd);
	return handle(); // FIXME
}



error_t
connector_pipe::close()
{
  if (!listening() && !connected()) {
    return ERR_INITIALIZATION;
  }

//  // We ignore errors from close() here. This is a problem with NFS, as the man
//  // pages state, but it's the price of the abstraction.
//  ::close(m_fd);
//  if (m_server) {
//    ::unlink(m_addr.full_str().c_str());
//  }
//
//  m_fd = -1;
//  m_server = false;

  return ERR_SUCCESS;
}


error_t
connector_pipe::set_blocking_mode(bool state)
{
//  return ::packeteer::set_blocking_mode(m_fd, state);
	return ERR_SUCCESS; // FIXME
}



error_t
connector_pipe::get_blocking_mode(bool & state) const
{
//  return ::packeteer::get_blocking_mode(m_fd, state);
	return ERR_SUCCESS; // FIXME
}


}} // namespace packeteer::detail
