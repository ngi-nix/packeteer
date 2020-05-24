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
#ifndef PACKETEER_CONNECTOR_WIN32_ANON_H
#define PACKETEER_CONNECTOR_WIN32_ANON_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#include "common.h"

namespace packeteer::detail {

/**
 * Unidirectional pipe; based on named pipe on WIN32
 **/
struct connector_anon : public connector_common
{
public:
  connector_anon(connector_options const & options);
  ~connector_anon();

  error_t listen();
  bool listening() const;

  error_t connect();
  bool connected() const;

  connector_interface * accept(net::socket_address & addr);

  handle get_read_handle() const;
  handle get_write_handle() const;

  error_t close();

  bool is_blocking() const;

  error_t receive(void * buf, size_t bufsize, size_t & bytes_read,
      ::packeteer::net::socket_address & sender);
  error_t send(void const * buf, size_t bufsize, size_t & bytes_written,
      ::packeteer::net::socket_address const & recipient);
  size_t peek() const;

private:
  error_t create_pipe();

  handle      m_handles[2];
  std::string m_addr;
};

} // namespace packeteer::detail

#endif // guard
