/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2019-2020 Jens Finkhaeuser.
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
#include <build-config.h>

#include <packeteer/connector/interface.h>

#include "common.h"
#include "io_operations.h"
#include "../../macros.h"

namespace packeteer::detail {

connector_common::connector_common(connector_options const & options)
  : m_options(options)
{
  DLOG("connector_common::connector_common(" << options << ")");
}



connector_common::~connector_common()
{
}



error_t
connector_common::read(void * buf, size_t bufsize, size_t & bytes_read)
{
  if (!connected() && !listening()) {
    return ERR_INITIALIZATION;
  }

  ssize_t read = -1;
  auto err = detail::read(get_read_handle(), buf, bufsize, read);
  if (ERR_SUCCESS == err) {
    bytes_read = read;
  }
  return err;
}



error_t
connector_common::write(void const * buf, size_t bufsize, size_t & bytes_written)
{
  if (!connected() && !listening()) {
    return ERR_INITIALIZATION;
  }

  ssize_t written = -1;
  auto err = detail::write(get_write_handle(), buf, bufsize, written);
  if (ERR_SUCCESS == err) {
    bytes_written = written;
  }
  return err;
}



connector_options
connector_common::get_options() const
{
  return m_options;
}


} // namespace packeteer::detail
