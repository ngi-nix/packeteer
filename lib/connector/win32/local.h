/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2020 Jens Finkhaeuser.
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
#ifndef PACKETEER_CONNECTOR_WIN32_LOCAL_H
#define PACKETEER_CONNECTOR_WIN32_LOCAL_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#include <packeteer/connector/interface.h>

#include "socket.h"

namespace packeteer::detail {

/**
 * Wrapper around the pipe class.
 **/
struct connector_local : public ::packeteer::detail::connector_socket
{
public:
  connector_local(peer_address const & addr,
      connector_options const & options);
  virtual ~connector_local();

  error_t listen() override;

  bool listening() const override;
  bool connected() const override;

  handle get_read_handle() const override;
  handle get_write_handle() const override;

  error_t connect() override;

  connector_interface * accept(liberate::net::socket_address & addr) override;

  error_t close() override;

private:
  connector_local();

  // File system entry owner.
  bool m_owner = false;

  ::packeteer::handle::sys_handle_t m_other_handle = ::packeteer::handle::INVALID_SYS_HANDLE;
};

} // namespace packeteer::detail

#endif // guard
