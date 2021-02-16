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
#ifndef PACKETEER_CONNECTOR_POSIX_COMMON_H
#define PACKETEER_CONNECTOR_POSIX_COMMON_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#include <packeteer/connector/interface.h>

namespace packeteer::detail {

/**
 * TCP connector
 **/
struct connector_common : public ::packeteer::connector_interface
{
public:
  explicit connector_common(connector_options const & options);
  virtual ~connector_common();

  // Common connector implementations
  error_t receive(void * buf, size_t bufsize, size_t & bytes_read,
      ::liberate::net::socket_address & sender) override;
  error_t send(void const * buf, size_t bufsize, size_t & bytes_written,
      ::liberate::net::socket_address const & recipient) override;
  size_t peek() const override;

  error_t read(void * buf, size_t bufsize, size_t & bytes_read) override;
  error_t write(void const * buf, size_t bufsize, size_t & bytes_written) override;

  connector_options get_options() const override;
protected:
  // Derived classes should be able to ues these directly.
  connector_options m_options;

private:
  connector_common();
};

} // namespace packeteer::detail

#endif // guard
