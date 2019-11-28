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
#ifndef PACKETEER_DETAIL_POSIX_CONNECTOR_SOCKET_H
#define PACKETEER_DETAIL_POSIX_CONNECTOR_SOCKET_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#include <packeteer/connector/interface.h>

#include <packeteer/net/socket_address.h>

namespace packeteer {
namespace detail {

/**
 * Base for socket-style I/O on POSIX systems
 **/
struct connector_socket : public ::packeteer::connector_interface
{
public:
  connector_socket(::packeteer::net::socket_address const & addr,
      connector_options const & options);

  // Connector interface, partially implemented
  bool listening() const;
  bool connected() const;

  handle get_read_handle() const;
  handle get_write_handle() const;

  error_t set_blocking_mode(bool state);
  error_t get_blocking_mode(bool & state) const;

  // Socket-specific versions of connect() and accept()
  error_t socket_create(int domain, int type, int & fd);
  error_t socket_bind(int domain, int type, int & fd);
  error_t socket_listen(int fd);
  error_t socket_connect(int domain, int type);
  error_t socket_accept(int & new_fd, net::socket_address & addr) const;
  error_t socket_close();

protected:
  connector_socket(connector_options const & options);

  ::packeteer::net::socket_address  m_addr;
  bool                              m_server;
  int                               m_fd;
};

}} // namespace packeteer::detail

#endif // guard
