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

#include <packeteer/net/socket_address.h>
#include <packeteer/error.h>

#include "../../globals.h"
#include "../../macros.h"

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

} // anonymous namespace




connector_local::connector_local(std::string const & path,
    connector_options const & options)
  : connector_socket(net::socket_address(path), options)
{
}



connector_local::connector_local(net::socket_address const & addr,
    connector_options const & options)
  : connector_socket(addr, options)
{
}



connector_local::~connector_local()
{
  close();
}



error_t
connector_local::connect()
{
  return connector_socket::socket_connect(AF_LOCAL, sock_type(m_options));
}



error_t
connector_local::listen()
{
  // Attempt to bind
  int fd = -1;
  error_t err = connector_socket::socket_bind(AF_LOCAL, sock_type(m_options), fd);
  if (ERR_SUCCESS != err) {
    return err;
  }
  m_owner = true;

  // Attempt to listen
  if (m_options & CO_STREAM) {
    err = connector_socket::socket_listen(fd);
    if (ERR_SUCCESS != err) {
      return err;
    }
  }

  // Finally, set the fd
  m_fd = fd;
  m_server = true;

  return ERR_SUCCESS;
}



error_t
connector_local::close()
{
  error_t err = connector_socket::socket_close();
  if (m_owner) {
    DLOG("Server closing; remove file system entry: " << m_addr.full_str());
    ::unlink(m_addr.full_str().c_str());
  }
  return err;
}



connector_interface *
connector_local::accept(net::socket_address & addr)
{
  int fd = -1;
  error_t err = connector_socket::socket_accept(fd, addr);
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
  result->m_fd = fd;

  return result;
}


} // namespace packeteer::detail
