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
#include <packeteer/detail/connector_udp.h>

#include <packeteer/net/socket_address.h>

#include <packeteer/detail/globals.h>

#include <packeteer/error.h>

namespace packeteer {
namespace detail {

namespace {

} // anonymous namespace

connector_udp::connector_udp(net::socket_address const & addr, bool blocking)
//  : connector_socket(addr, blocking, CO_DATAGRAM)
{
}



connector_udp::connector_udp()
//  : connector_socket()
{
}



connector_udp::~connector_udp()
{
  close();
}



error_t
connector_udp::connect()
{
//  return connector_socket::socket_connect(select_domain(m_addr), SOCK_DGRAM);
	return ERR_SUCCESS; // FIXME
}



error_t
connector_udp::listen()
{
//  // Attempt to bind
//  int fd = -1;
//  error_t err = connector_socket::socket_bind(select_domain(m_addr), SOCK_DGRAM, fd);
//  if (ERR_SUCCESS != err) {
//    return err;
//  }
//
//  // Finally, set the fd
//  m_fd = fd;
//  m_server = true;

  return ERR_SUCCESS;
}



error_t
connector_udp::close()
{
  return connector_socket::socket_close();
}


connector *
connector_udp::accept(net::socket_address & /* addr*/)
{
  if (!listening()) {
    return nullptr;
  }
  return this;
}


}} // namespace packeteer::detail
