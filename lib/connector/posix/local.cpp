/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2014 Unwesen Ltd.
 * Copyright (c) 2015-2020 Jens Finkhaeuser.
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

#include <packeteer/error.h>

#include "../../globals.h"
#include "../../macros.h"
#include "fd.h"

#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>


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
    int & server,
    int & client,
    bool & done)
{
  if (addr.type() != liberate::net::AT_UNSPEC) {
    done = false;
    return ERR_UNEXPECTED; // Doesn't matter
  }

#if defined(PACKETEER_HAVE_SOCKETPAIR)

  int sockets[2];
  auto err = socketpair(AF_UNIX, sock_type(options), 0, sockets);
  if (ERR_SUCCESS != err) {
    done = false;
    return err;
  }

  // Great, assign the sockets to the fds.
  auto e = detail::set_blocking_mode(sockets[0], options & CO_BLOCKING);
  if (ERR_SUCCESS != e) {
    close(sockets[0]);
    close(sockets[1]);
    return e;
  }
  e = detail::set_blocking_mode(sockets[1], options & CO_BLOCKING);
  if (ERR_SUCCESS != e) {
    close(sockets[0]);
    close(sockets[1]);
    return e;
  }

  server = sockets[0];
  client = sockets[1];

  done = true;
  return ERR_SUCCESS;
#else
  ELOG("No socketpair() implementation found.");
  return ERR_INVALID_OPTION;
#endif
}



} // anonymous namespace



connector_local::connector_local(peer_address const & addr,
    connector_options const & options)
  : connector_socket{addr, options}
{
}



connector_local::~connector_local()
{
  connector_local::close();
}



bool
connector_local::listening() const
{
  if (m_address.socket_address().type() == liberate::net::AT_UNSPEC) {
    return m_fd != -1 && m_other_fd != -1;
  }
  return connector_socket::listening();
}



bool
connector_local::connected() const
{
  if (m_address.socket_address().type() == liberate::net::AT_UNSPEC) {
    return m_fd != -1 && m_other_fd != -1;

  }
  return connector_socket::connected();
}



handle
connector_local::get_read_handle() const
{
  // Always return this as the read handle; this way, we only need to do
  // anything special in the get_write_handle() function.
  return m_fd;
}



handle
connector_local::get_write_handle() const
{
  if (m_address.socket_address().type() == liberate::net::AT_UNSPEC) {
    return m_other_fd;
  }
  return m_fd;
}



error_t
connector_local::connect()
{
  if (connected()) {
    return ERR_INITIALIZATION;
  }

  // Deal with unnamed sockets
  bool done = false;
  auto err = create_socketpair(m_address.socket_address(), m_options, m_fd, m_other_fd,
      done);
  if (done) {
    m_server = true;
    return err;
  }

  return connector_socket::socket_connect(AF_LOCAL, sock_type(m_options));
}



error_t
connector_local::listen()
{
  if (listening()) {
    return ERR_INITIALIZATION;
  }

  // Deal with unnamed sockets
  bool done = false;
  auto err = create_socketpair(m_address.socket_address(), m_options, m_fd, m_other_fd,
      done);
  if (done) {
    ERR_LOG("Created socketpair", err);
    return err;
  }

  // Attempt to bind
  int fd = -1;
  err = connector_socket::socket_bind(AF_LOCAL, sock_type(m_options),
      fd);
  if (ERR_SUCCESS != err) {
    ERR_LOG("Bind failed", err);
    return err;
  }
  m_owner = true;

  // Attempt to listen
  if (m_options & CO_STREAM) {
    err = connector_socket::socket_listen(fd);
    if (ERR_SUCCESS != err) {
      ERR_LOG("Listen failed", err);
      return err;
    }
  }

  // Finally, set the fd
  m_fd = fd;
  m_server = true;
  DLOG("Now listening.");

  return ERR_SUCCESS;
}



error_t
connector_local::close()
{
  error_t err = connector_socket::socket_close();

  if (m_owner) {
    auto str = m_address.socket_address().full_str();
    if (str[0] != '\0') {
      DLOG("Server closing; remove file system entry: " << m_address.socket_address());
      auto ret = ::unlink(m_address.socket_address().full_str().c_str());
      if (ret < 0) {
        ERRNO_LOG("Unlink failed");
      }
    }
  }

  if (m_other_fd != -1) {
    ::close(m_other_fd);
    m_other_fd = -1;
  }

  return err;
}



connector_interface *
connector_local::accept(liberate::net::socket_address & addr)
{
  if (!listening()) {
    return nullptr;
  }

  if (m_other_fd != -1) {
    // No need for accept() if we're a socket pair
    return this;
  }

  int fd = -1;
  error_t err = connector_socket::socket_accept(fd, addr);
  if (ERR_SUCCESS != err) {
    return nullptr;
  }
  addr = m_address.socket_address();

  // Create & return connector with accepted FD. Only the instance
  // that bound the socket is the file system entry owner, though.
  auto res_addr = m_address;
  res_addr.socket_address() = addr;
  connector_local * result = new connector_local{res_addr, m_options};
  result->m_server = true;
  result->m_connected = true;
  result->m_owner = false;
  result->m_fd = fd;

  return result;
}


} // namespace packeteer::detail
