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
 * This software is distributed on an "AS IS" BASIS, WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.
 **/
#include <build-config.h>

#include "tcp.h"

#include <packeteer/error.h>

#include "../../globals.h"
#include "../../macros.h"

#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>


namespace packeteer::detail {

namespace {

inline int
select_domain(liberate::net::socket_address const & addr)
{
  switch (addr.type()) {
    case ::liberate::net::AT_INET4:
      return AF_INET;

    case ::liberate::net::AT_INET6:
      return AF_INET6;

    default:
      throw exception(ERR_INVALID_VALUE, "Expected IPv4 or IPv6 address!");
  }
}

} // anonymous namespace

connector_tcp::connector_tcp(peer_address const & addr,
    connector_options const & options)
  : connector_socket{addr, options}
{
}



connector_tcp::~connector_tcp()
{
  connector_tcp::close();
}



error_t
connector_tcp::connect()
{
  return connector_socket::socket_connect(
      select_domain(m_address.socket_address()),
      SOCK_STREAM);
}



error_t
connector_tcp::listen()
{
  // Attempt to bind
  int fd = -1;
  error_t err = connector_socket::socket_bind(
      select_domain(m_address.socket_address()),
      SOCK_STREAM, fd);
  if (ERR_SUCCESS != err) {
    return err;
  }

  // Attempt to listen
  err = connector_socket::socket_listen(fd);
  if (ERR_SUCCESS != err) {
    return err;
  }

  // Finally, set the fd
  m_fd = fd;
  m_server = true;

  return ERR_SUCCESS;
}



error_t
connector_tcp::close()
{
  return connector_socket::socket_close();
}


connector_interface *
connector_tcp::accept(liberate::net::socket_address & addr)
{
  int fd = -1;
  error_t err = connector_socket::socket_accept(fd, addr);
  if (ERR_SUCCESS != err) {
    return nullptr;
  }

  // Create & return connector with accepted FD
  auto res_addr = m_address;
  res_addr.socket_address() = addr;
  connector_tcp * result = new connector_tcp{res_addr, m_options};
  result->m_server = true;
  result->m_connected = true;
  result->m_fd = fd;

  return result;
}


} // namespace packeteer::detail
