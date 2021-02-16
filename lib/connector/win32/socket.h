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
#ifndef PACKETEER_CONNECTOR_WIN32_SOCKET_H
#define PACKETEER_CONNECTOR_WIN32_SOCKET_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#include "common.h"

#include "../../net/netincludes.h"

namespace packeteer::detail {

/**
 * Utility functions for connector_socket() and socketpair.h
 */
PACKETEER_PRIVATE
error_t
create_socket(int domain, int type, int proto, SOCKET & socket,
    bool blocking);


PACKETEER_PRIVATE
void
close_socket(SOCKET socket);


PACKETEER_PRIVATE
error_t
set_blocking(SOCKET socket, bool blocking);


/**
 * Base for socket-style I/O on Win32
 **/
struct connector_socket : public connector_common
{
public:
  connector_socket(::liberate::net::socket_address const & addr,
      connector_options const & options);

  // Connector interface, partially implemented
  virtual bool listening() const override;
  virtual bool connected() const override;

  virtual handle get_read_handle() const override;
  virtual handle get_write_handle() const override;

  bool is_blocking() const override;

  error_t receive(void * buf, size_t bufsize, size_t & bytes_read,
      ::liberate::net::socket_address & sender) override;
  error_t send(void const * buf, size_t bufsize, size_t & bytes_written,
      ::liberate::net::socket_address const & recipient) override;
  size_t peek() const override;

  // Socket-specific versions of connect() and accept()
  error_t socket_create(int domain, int type, int proto,
      handle::sys_handle_t & h);
  error_t socket_bind(int domain, int type, int proto,
      handle::sys_handle_t & h);
  error_t socket_listen(handle::sys_handle_t h);
  error_t socket_connect(int domain, int type, int proto);
  error_t socket_accept(handle::sys_handle_t & new_handle, liberate::net::socket_address & addr);
  error_t socket_close();

protected:
  explicit connector_socket(connector_options const & options);

  ::liberate::net::socket_address   m_addr = {};
  bool                              m_server = false;
  bool                              m_connected = false;
  ::packeteer::handle::sys_handle_t m_handle = ::packeteer::handle::INVALID_SYS_HANDLE;
};

} // namespace packeteer::detail

#endif // guard
