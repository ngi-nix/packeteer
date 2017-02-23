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

#include <packeteer/detail/connector.h>

#include <packeteer/net/socket_address.h>

namespace packeteer {
namespace detail {

/**
 * Wrapper around the pipe class.
 **/
struct connector_local : public ::packeteer::detail::connector
{
public:
  connector_local(std::string const & path);
  connector_local(::packeteer::net::socket_address const & addr);
  ~connector_local();

  error_t listen();
  bool listening() const;

  error_t connect();
  bool connected() const;

  connector * accept(net::socket_address & addr) const;

  int get_read_fd() const;
  int get_write_fd() const;

  error_t close();
private:

  connector_local();

  ::packeteer::net::socket_address  m_addr;
  bool                              m_server;
  int                               m_fd;
};

}} // namespace packeteer::detail

#endif // guard
