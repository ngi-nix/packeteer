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
 * This software is distributed on an "AS IS" BASIS, WITHOUT ANY WARRANTY
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.
 **/
#include <packeteer/detail/connector_anon.h>

#include <packeteer/detail/filedescriptors.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>


namespace packeteer {
namespace detail {

connector_anon::connector_anon(bool block /* = false */)
  : m_block(block)
{
  m_fds[0] = m_fds[1] = -1;
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
  int ret = ::pipe(m_fds);
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
  if (ERR_SUCCESS != make_nonblocking(m_fds[0], m_block)) {
    close();
    return ERR_UNEXPECTED;
  }
  if (ERR_SUCCESS != make_nonblocking(m_fds[1], m_block)) {
    close();
    return ERR_UNEXPECTED;
  }

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
  return (m_fds[0] != -1 && m_fds[1] != -1);
}



connector *
connector_anon::accept(net::socket_address & addr) const
{
  // There is no need for accept(); we've already got the connection established.
  if (!connected()) {
    return nullptr;
  }
  return const_cast<connector_anon*>(this);
}



int
connector_anon::get_read_fd() const
{
  return m_fds[0];
}



int
connector_anon::get_write_fd() const
{
  return m_fds[1];
}



error_t
connector_anon::close()
{
  if (!connected()) {
    return ERR_INITIALIZATION;
  }

  // We ignore errors from close() here. This is a problem with NFS, as the man
  // pages state, but it's the price of the abstraction.
  ::close(m_fds[0]);
  ::close(m_fds[1]);

  m_fds[0] = m_fds[1] = -1;

  return ERR_SUCCESS;
}

}} // namespace packeteer::detail
