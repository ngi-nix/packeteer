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
 * This software is distributed on an "AS IS" BASIS, WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.
 **/
#ifndef PACKETEER_DETAIL_CONNECTOR_LOCAL_H
#define PACKETEER_DETAIL_CONNECTOR_LOCAL_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer/packeteer.h>
#include <packeteer/connector_specs.h>

#include <packeteer/detail/connector.h>

// FIXME this header should probably be in POSIX for inherting from connector_socket
#include <packeteer/posix/detail/connector_socket.h>

#include <packeteer/net/socket_address.h>

namespace packeteer {
namespace detail {

/**
 * Wrapper around the pipe class.
 **/
struct connector_local : public ::packeteer::detail::connector_socket
{
public:
  connector_local(std::string const & path, bool blocking,
      connector_behaviour const & behaviour);
  connector_local(::packeteer::net::socket_address const & addr, bool blocking,
      connector_behaviour const & behaviour);
  ~connector_local();

  error_t listen();

  error_t connect();

  connector * accept(net::socket_address & addr) const;

  error_t close();

private:
  connector_local();
};

}} // namespace packeteer::detail

#endif // guard
