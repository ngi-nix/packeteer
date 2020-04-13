/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2014 Unwesen Ltd.
 * Copyright (c) 2015-2019 Jens Finkhaeuser.
 *
 * This software is licensed under the terms of the GNU GPLv3 for personal,
 * educational and non-profit use. For all other uses, alternative license
 * options are available. Please contact the copyright holder for additional
 * information, stating your intended usage.
 *
 * You can find the full text of the GPLv3 in the COPYING file in this code
 * distribution.
 *
 * This software is distributed on an "AS IS" BASIS, WITHOUT ANY WARRANTY
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.
 **/
#include <build-config.h>

#include "../anon.h"

#include "fd.h"
#include "../../macros.h"

#include <packeteer/handle.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>


namespace packeteer::detail {

connector_anon::connector_anon(connector_options const & options)
  : connector_interface(CO_STREAM | (options & CO_BLOCKING))
{
}



connector_anon::~connector_anon()
{
  close();
}



error_t
connector_anon::create_pipe()
{
  if (connected()) {
    return ERR_INITIALIZATION;
  }

  // Create pipe
  int fds[2] = { -1, -1 };

  int ret = ::pipe(fds);
  if (-1 == ret) {
    ERRNO_LOG("connector_anon pipe failed!");
    switch (errno) {
      case EMFILE:
      case ENFILE:
        close();
        return ERR_NUM_FILES;
        break;

      default:
        close();
        return ERR_UNEXPECTED;
        break;
    }
  }

  // Optionally make the read and write end non-blocking
  if (ERR_SUCCESS != detail::set_blocking_mode(fds[0],
        m_options & CO_BLOCKING))
  {
    close();
    return ERR_UNEXPECTED;
  }
  if (ERR_SUCCESS != detail::set_blocking_mode(fds[1],
        m_options & CO_BLOCKING))
  {
    close();
    return ERR_UNEXPECTED;
  }

  m_handles[0] = handle{fds[0]};
  m_handles[1] = handle{fds[1]};

  return ERR_SUCCESS;
}



error_t
connector_anon::listen()
{
  return create_pipe();
}



bool
connector_anon::listening() const
{
  return connected();
}



error_t
connector_anon::connect()
{
  return create_pipe();
}



bool
connector_anon::connected() const
{
  return m_handles[0].valid() && m_handles[1].valid();
}



connector_interface *
connector_anon::accept(net::socket_address & /* unused */)
{
  // There is no need for accept(); we've already got the connection established.
  if (!connected()) {
    return nullptr;
  }
  return this;
}



handle
connector_anon::get_read_handle() const
{
  return m_handles[0];
}



handle
connector_anon::get_write_handle() const
{
  return m_handles[1];
}



error_t
connector_anon::close()
{
  if (!connected()) {
    return ERR_INITIALIZATION;
  }

  // We ignore errors from close() here. This is a problem with NFS, as the man
  // pages state, but it's the price of the abstraction.
  ::close(m_handles[0].sys_handle());
  ::close(m_handles[1].sys_handle());

  m_handles[0] = m_handles[1] = handle{};

  return ERR_SUCCESS;
}



bool
connector_anon::is_blocking() const
{
  bool states[2] = { false, false };
  error_t err = detail::get_blocking_mode(m_handles[0].sys_handle(), states[0]);
  if (ERR_SUCCESS != err) {
    throw exception(err, "Could not determine blocking mode from file descriptor!");
  }
  err = detail::get_blocking_mode(m_handles[1].sys_handle(), states[1]);
  if (ERR_SUCCESS != err) {
    throw exception(err, "Could not determine blocking mode from file descriptor!");
  }

  if (states[0] != states[1]) {
    throw exception(ERR_UNEXPECTED, "The two file descriptors had differing blocking modes.");
  }

  return states[0];
}


} // namespace packeteer::detail
