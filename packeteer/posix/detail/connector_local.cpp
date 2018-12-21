/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2014 Unwesen Ltd.
 * Copyright (c) 2015-2019 Jens Finkhaeuser.
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
#include <packeteer/detail/connector_local.h>

#include <packeteer/net/socket_address.h>

#include <packeteer/detail/globals.h>

#include <packeteer/error.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>


namespace packeteer {
namespace detail {

namespace {

inline int
sock_type(connector_behaviour const & behaviour)
{
  switch (behaviour) {
    case CB_DATAGRAM:
      return SOCK_DGRAM;
    case CB_STREAM:
    default:
      return SOCK_STREAM;
  }
}

} // anonymous namespace




connector_local::connector_local(std::string const & path, bool blocking,
    connector_behaviour const & behaviour)
  : connector_socket(net::socket_address(path), blocking, behaviour)
{
}



connector_local::connector_local(net::socket_address const & addr, bool blocking,
    connector_behaviour const & behaviour)
  : connector_socket(addr, blocking, behaviour)
{
}



connector_local::connector_local()
  : connector_socket()
{
}



connector_local::~connector_local()
{
  close();
}



error_t
connector_local::connect()
{
  return connector_socket::connect(AF_LOCAL, sock_type(m_behaviour));
}



error_t
connector_local::listen()
{
  // Attempt to bind
  int fd = -1;
  error_t err = connector_socket::bind(AF_LOCAL, sock_type(m_behaviour), fd);
  if (ERR_SUCCESS != err) {
    return err;
  }

  // Attempt to listen
  err = connector_socket::listen(fd);
  if (ERR_SUCCESS != err) {
    return err;
  }

  // Finally, set the fd
  m_fd = fd;
  m_server = true;

  return ERR_SUCCESS;
}



error_t
connector_local::close()
{
  bool server = m_server;
  error_t err = connector_socket::close_socket();
  if (ERR_SUCCESS == err && server) {
    ::unlink(m_addr.full_str().c_str());
  }
  return err;
}



connector *
connector_local::accept(net::socket_address & addr) const
{
  int fd = -1;
  error_t err = connector_socket::accept(fd, addr);
  if (ERR_SUCCESS != err) {
    return nullptr;
  }

  // Create & return connector with accepted FD
  connector_local * result = new connector_local();
  result->m_addr = addr;
  result->m_server = true;
  result->m_fd = fd;
  result->m_behaviour = m_behaviour;

  return result;
}


}} // namespace packeteer::detail
