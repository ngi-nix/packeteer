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
 * This software is distributed on an "AS IS" BASIS, WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.
 **/
#ifndef PACKETEER_CONNECTOR_POSIX_LOCAL_H
#define PACKETEER_CONNECTOR_POSIX_LOCAL_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#include <packeteer/connector/interface.h>

#include <packeteer/net/socket_address.h>

#include "socket.h"

namespace packeteer::detail {

/**
 * Wrapper around the pipe class.
 **/
struct connector_local : public ::packeteer::detail::connector_socket
{
public:
  connector_local(std::string const & path,
      connector_options const & options);
  connector_local(::packeteer::net::socket_address const & addr,
      connector_options const & options);
  virtual ~connector_local();

  error_t listen();

  error_t connect();

  connector_interface * accept(net::socket_address & addr);

  error_t close();

private:
  connector_local();

  // File system entry owner.
  bool m_owner = false;
};

} // namespace packeteer::detail

#endif // guard
