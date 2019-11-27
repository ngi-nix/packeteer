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
#ifndef PACKETEER_DETAIL_CONNECTOR_UDP_H
#define PACKETEER_DETAIL_CONNECTOR_UDP_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#include <packeteer/net/socket_address.h>

// FIXME this header should probably be in POSIX for inherting from connector_socket
#include "../posix/detail/connector_socket.h"
#include "connector.h"


namespace packeteer {
namespace detail {

/**
 * Wrapper around the pipe class.
 **/
struct connector_udp : public ::packeteer::detail::connector_socket
{
public:
  connector_udp(::packeteer::net::socket_address const & addr,
      connector_options const & options);
  ~connector_udp();

  error_t listen();

  error_t connect();

  connector * accept(net::socket_address & addr) const;

  error_t close();

private:
  connector_udp();
};

}} // namespace packeteer::detail

#endif // guard
