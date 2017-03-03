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
#ifndef PACKETEER_DETAIL_POSIX_CONNECTOR_SOCKET_H
#define PACKETEER_DETAIL_POSIX_CONNECTOR_SOCKET_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer/packeteer.h>

#include <packeteer/detail/connector.h>

#include <packeteer/net/socket_address.h>

namespace packeteer {
namespace detail {

/**
 * Base for socket-style I/O on POSIX systems
 **/
struct connector_socket : public ::packeteer::detail::connector
{
public:
  connector_socket(::packeteer::net::socket_address const & addr);

  error_t bind(int domain, int type, int & fd);
  error_t listen(int fd);
  bool listening() const;

  error_t connect(int domain, int type);
  bool connected() const;

  error_t accept(int & new_fd, net::socket_address & addr) const;

  handle get_read_handle() const;
  handle get_write_handle() const;

  error_t close_socket();

protected:
  connector_socket();

  ::packeteer::net::socket_address  m_addr;
  bool                              m_server;
  int                               m_fd;
};

}} // namespace packeteer::detail

#endif // guard
