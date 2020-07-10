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
#ifndef PACKETEER_CONNECTOR_WIN32_PIPE_H
#define PACKETEER_CONNECTOR_WIN32_PIPE_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#include <packeteer/handle.h>

#include "common.h"

namespace packeteer::detail {

/**
 * Wrapper around the pipe class.
 **/
struct connector_pipe : public connector_common
{
public:
  connector_pipe(std::string const & path, connector_options const & options);
  connector_pipe(::liberate::net::socket_address const & addr,
      connector_options const & options);
  ~connector_pipe();

  error_t listen() override;
  bool listening() const override;

  error_t connect() override;
  bool connected() const override;

  connector_interface * accept(liberate::net::socket_address & addr) override;

  handle get_read_handle() const override;
  handle get_write_handle() const override;

  error_t close() override;

  bool is_blocking() const override;

  error_t receive(void * buf, size_t bufsize, size_t & bytes_read,
      ::liberate::net::socket_address & sender) override;
  error_t send(void const * buf, size_t bufsize, size_t & bytes_written,
      ::liberate::net::socket_address const & recipient) override;
  size_t peek() const override;


private:
  connector_pipe();

  ::liberate::net::socket_address   m_addr = {};
  bool                              m_server = false;
  bool                              m_owner = false;
  bool                              m_connected = false;
  ::packeteer::handle               m_handle = {};
};

} // namespace packeteer::detail

#endif // guard
