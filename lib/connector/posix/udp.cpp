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

#include "udp.h"

#include <packeteer/error.h>

#include "../../globals.h"

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

connector_udp::connector_udp(peer_address const & addr,
    connector_options const & options)
  : connector_socket{addr, options}
{
}



connector_udp::~connector_udp()
{
  connector_udp::close();
}



error_t
connector_udp::connect()
{
  return connector_socket::socket_connect(
      select_domain(m_address.socket_address()),
      SOCK_DGRAM);
}



error_t
connector_udp::listen()
{
  // Attempt to bind
  int fd = -1;
  error_t err = connector_socket::socket_bind(
      select_domain(m_address.socket_address()),
      SOCK_DGRAM, fd);
  if (ERR_SUCCESS != err) {
    return err;
  }

  // Finally, set the fd
  m_fd = fd;
  m_server = true;

  return ERR_SUCCESS;
}



error_t
connector_udp::close()
{
  return connector_socket::socket_close();
}


connector_interface *
connector_udp::accept(liberate::net::socket_address & /* addr*/)
{
  if (!listening()) {
    return nullptr;
  }
  return this;
}


} // namespace packeteer::detail
