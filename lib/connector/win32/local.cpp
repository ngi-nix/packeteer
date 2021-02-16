/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2020 Jens Finkhaeuser.
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

#include "local.h"

#include <liberate/string/utf8.h>

#include <packeteer/error.h>

#include "../../globals.h"
#include "../../macros.h"
#include "../../win32/sys_handle.h"
#include "socketpair.h"

namespace packeteer::detail {

namespace {


inline int
sock_type(connector_options const & options)
{
  if (options & CO_DATAGRAM) {
    return SOCK_DGRAM;
  }
  return SOCK_STREAM;
}



inline error_t
create_socketpair(
    liberate::net::socket_address const & addr,
    connector_options const & options,
    handle::sys_handle_t & server,
    handle::sys_handle_t & client,
    bool & done)
{
  if (addr.type() != liberate::net::AT_UNSPEC) {
    done = false;
    return ERR_UNEXPECTED; // Doesn't matter
  }

  SOCKET sockets[2];
  auto err = socketpair(AF_UNIX, sock_type(options), 0, sockets);
  if (ERR_SUCCESS != err) {
    done = false;
    return err;
  }

  // Great, assign the sockets to the handles.
  server = std::make_shared<handle::opaque_handle>(sockets[0]);
  server->blocking = options & CO_BLOCKING;
  client = std::make_shared<handle::opaque_handle>(sockets[1]);
  client->blocking = options & CO_BLOCKING;
  done = true;
  return ERR_SUCCESS;
}


} // anonymous namespace



connector_local::connector_local(liberate::net::socket_address const & addr,
    connector_options const & options)
  : connector_socket(addr, options)
{
}



connector_local::~connector_local()
{
  connector_local::close();
}



error_t
connector_local::connect()
{
  if (connected()) {
    return ERR_INITIALIZATION;
  }

  // Deal with unnamed sockets
  bool done = false;
  auto err = create_socketpair(m_addr, m_options, m_handle, m_other_handle,
      done);
  if (done) {
    m_server = true;
    return err;
  }

  return connector_socket::socket_connect(AF_UNIX, sock_type(m_options), 0);
}



bool
connector_local::listening() const
{
  if (m_addr.type() == liberate::net::AT_UNSPEC) {
    return m_handle != handle::INVALID_SYS_HANDLE &&
      m_other_handle != handle::INVALID_SYS_HANDLE;
  }
  return connector_socket::listening();
}



bool
connector_local::connected() const
{
  if (m_addr.type() == liberate::net::AT_UNSPEC) {
    return m_handle != handle::INVALID_SYS_HANDLE &&
      m_other_handle != handle::INVALID_SYS_HANDLE;

  }
  return connector_socket::connected();
}



handle
connector_local::get_read_handle() const
{
  // Always return this as the read handle; this way, we only need to do
  // anything special in the get_write_handle() function.
  return m_handle;
}



handle
connector_local::get_write_handle() const
{
  if (m_addr.type() == liberate::net::AT_UNSPEC) {
    return m_other_handle;
  }
  return m_handle;
}



error_t
connector_local::listen()
{
  if (listening()) {
    return ERR_INITIALIZATION;
  }

  // Deal with unnamed sockets
  bool done = false;
  auto err = create_socketpair(m_addr, m_options, m_handle, m_other_handle,
      done);
  if (done) {
    return err;
  }

  // Attempt to bind
  handle::sys_handle_t handle = handle::INVALID_SYS_HANDLE;
  err = connector_socket::socket_bind(AF_UNIX, sock_type(m_options),
      0, handle);
  if (ERR_SUCCESS != err) {
    return err;
  }
  m_owner = true;

  // Attempt to listen
  if (m_options & CO_STREAM) {
    err = connector_socket::socket_listen(handle);
    if (ERR_SUCCESS != err) {
      return err;
    }
  }

  // Finally, set the fd
  m_handle = handle;
  m_server = true;

  return ERR_SUCCESS;
}



error_t
connector_local::close()
{
  error_t err = connector_socket::socket_close();
  if (m_owner) {
    DLOG("Server closing; remove file system entry: " << m_addr.full_str());
    DeleteFile(liberate::string::from_utf8(m_addr.full_str().c_str()).c_str());
  }

  if (m_other_handle != handle::INVALID_SYS_HANDLE) {
    close_socket(m_other_handle->socket);
    m_other_handle = handle::INVALID_SYS_HANDLE;
  }

  return err;
}



connector_interface *
connector_local::accept(liberate::net::socket_address & addr)
{
  if (!listening()) {
    return nullptr;
  }

  if (m_other_handle != handle::INVALID_SYS_HANDLE) {
    // No need for accept() if we're a socket pair
    return this;
  }

  handle::sys_handle_t handle = handle::INVALID_SYS_HANDLE;
  error_t err = connector_socket::socket_accept(handle, addr);
  if (ERR_SUCCESS != err) {
    return nullptr;
  }
  addr = m_addr;

  // Create & return connector with accepted FD. Only the instance
  // that bound the socket is the file system entry owner, though.
  connector_local * result = new connector_local(m_addr, m_options);
  result->m_server = true;
  result->m_connected = true;
  result->m_owner = false;
  result->m_handle = handle;

  return result;
}


} // namespace packeteer::detail
