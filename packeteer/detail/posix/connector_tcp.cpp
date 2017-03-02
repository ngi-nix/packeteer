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
 * This software is distributed on an "AS IS" BASIS, WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.
 **/
#include <packeteer/detail/connector_tcp.h>

#include <packeteer/net/socket_address.h>

#include <packeteer/detail/filedescriptors.h>
#include <packeteer/detail/globals.h>

#include <packeteer/error.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>


namespace packeteer {
namespace detail {

connector_tcp::connector_tcp(net::socket_address const & addr)
  : connector_socket(addr)
{
}



connector_tcp::connector_tcp()
  : connector_socket()
{
}



connector_tcp::~connector_tcp()
{
  close();
}



error_t
connector_tcp::connect()
{
  return connector_socket::connect(PF_INET, SOCK_STREAM);
}



error_t
connector_tcp::listen()
{
  return connector_socket::listen(PF_INET, SOCK_STREAM);
}



error_t
connector_tcp::close()
{
  return connector_socket::close_socket();
}


connector *
connector_tcp::accept(net::socket_address & addr) const
{
  int fd = -1;
  error_t err = connector_socket::accept(fd, addr);
  if (ERR_SUCCESS != err) {
    return nullptr;
  }

  // Create & return connector with accepted FD
  connector_tcp * result = new connector_tcp();
  result->m_addr = addr;
  result->m_server = true;
  result->m_fd = fd;

  return result;
}


}} // namespace packeteer::detail
