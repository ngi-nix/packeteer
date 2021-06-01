/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2017-2020 Jens Finkhaeuser.
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
#ifndef PACKETEER_CONNECTOR_POSIX_SOCKET_H
#define PACKETEER_CONNECTOR_POSIX_SOCKET_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#include "common.h"

namespace packeteer::detail {

/**
 * Base for socket-style I/O on POSIX systems
 **/
struct connector_socket : public ::packeteer::detail::connector_common
{
public:
  connector_socket(peer_address const & addr,
      connector_options const & options);

  // Connector interface, partially implemented
  bool listening() const override;
  bool connected() const override;

  handle get_read_handle() const override;
  handle get_write_handle() const override;

  bool is_blocking() const override;

  // Socket-specific versions of connect() and accept()
  error_t socket_create(int domain, int type, int & fd);
  error_t socket_bind(int domain, int type, int & fd);
  error_t socket_listen(int fd);
  error_t socket_connect(int domain, int type);
  error_t socket_accept(int & new_fd, liberate::net::socket_address & addr);
  error_t socket_close();

protected:
  bool                              m_server = false;
  bool                              m_connected = false;
  int                               m_fd = -1;
};

} // namespace packeteer::detail

#endif // guard
