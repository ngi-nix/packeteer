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
 * This software is distributed on an "AS IS" BASIS, WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.
 **/
#include <build-config.h>

#include "tcp.h"

namespace packeteer::detail {

namespace {

inline int
select_domain(::packeteer::net::socket_address const & addr)
{
  switch (addr.type()) {
    case ::packeteer::net::AT_INET4:
      return AF_INET;

    case ::packeteer::net::AT_INET6:
      return AF_INET6;

    default:
      throw exception(ERR_INVALID_VALUE, "Expected IPv4 or IPv6 address!");
  }
}

} // anonymous namespace

connector_tcp::connector_tcp(net::socket_address const & addr,
    connector_options const & options)
  : connector_socket(addr, options)
{
}



connector_tcp::~connector_tcp()
{
  connector_tcp::close();
}



error_t
connector_tcp::connect()
{
  return connector_socket::socket_connect(select_domain(m_addr), SOCK_STREAM,
      IPPROTO_TCP);
}



error_t
connector_tcp::listen()
{
  // Attempt to bind
  handle::sys_handle_t handle = handle::INVALID_SYS_HANDLE;
  error_t err = connector_socket::socket_bind(select_domain(m_addr),
      SOCK_STREAM, IPPROTO_TCP, handle);
  if (ERR_SUCCESS != err) {
    return err;
  }

  // Attempt to listen
  err = connector_socket::socket_listen(handle);
  if (ERR_SUCCESS != err) {
    return err;
  }

  // Finally, set the handle
  m_handle = handle;
  m_server = true;

  return ERR_SUCCESS;
}



error_t
connector_tcp::close()
{
  return connector_socket::socket_close();
}


connector_interface *
connector_tcp::accept(net::socket_address & addr)
{
  handle::sys_handle_t handle = handle::INVALID_SYS_HANDLE;
  error_t err = connector_socket::socket_accept(handle, addr);
  if (ERR_SUCCESS != err) {
    return nullptr;
  }

  // Create & return connector with accepted FD
  connector_tcp * result = new connector_tcp(addr, m_options);
  result->m_server = true;
  result->m_connected = true;
  result->m_handle = handle;

  return result;
}


} // namespace packeteer::detail
