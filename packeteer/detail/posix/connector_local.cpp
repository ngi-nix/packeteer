/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2014 Unwesen Ltd.
 * Copyright (c) 2015-2017 Jens Finkhaeuser.
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

#include <packeteer/detail/filedescriptors.h>
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

connector_local::connector_local(std::string const & path)
  : connector_socket(path)
{
}



connector_local::connector_local(net::socket_address const & addr)
  : connector_socket(addr)
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
  return connector_socket::connect(PF_LOCAL, SOCK_STREAM);
}



error_t
connector_local::listen()
{
  // Attempt to bind
  int fd = -1;
  error_t err = connector_socket::bind(PF_LOCAL, SOCK_STREAM, fd);
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
  // FIXME use accept for CB_DATAGRAM?
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

  return result;
}


}} // namespace packeteer::detail
