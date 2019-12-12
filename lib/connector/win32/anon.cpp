/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2019 Jens Finkhaeuser.
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

#include <packeteer/handle.h>


namespace packeteer {
namespace detail {

connector_anon::connector_anon(bool blocking)
 : connector(blocking, CO_STREAM)
{
// FIXME  m_fds[0] = m_fds[1] = -1;
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
// FIXME
// FIXME  // Create pipe
// FIXME  int ret = ::pipe(m_fds);
// FIXME  if (-1 == ret) {
// FIXME    ERRNO_LOG("connector_anon pipe failed!");
// FIXME    switch (errno) {
// FIXME      case EMFILE:
// FIXME      case ENFILE:
// FIXME        close();
// FIXME        return ERR_NUM_FILES;
// FIXME        break;
// FIXME
// FIXME      default:
// FIXME        close();
// FIXME        return ERR_UNEXPECTED;
// FIXME        break;
// FIXME    }
// FIXME  }
// FIXME
// FIXME  // Optionally make the read and write end non-blocking
// FIXME  if (ERR_SUCCESS != ::packeteer::set_blocking_mode(m_fds[0], m_blocking)) {
// FIXME    close();
// FIXME    return ERR_UNEXPECTED;
// FIXME  }
// FIXME  if (ERR_SUCCESS != ::packeteer::set_blocking_mode(m_fds[1], m_blocking)) {
// FIXME    close();
// FIXME    return ERR_UNEXPECTED;
// FIXME  }

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
//  return (m_fds[0] != -1 && m_fds[1] != -1);
	return false; // FIXME
}



connector *
connector_anon::accept(net::socket_address & /* unused */) const
{
  // There is no need for accept(); we've already got the connection established.
	// FIXME
  if (!connected()) {
    return nullptr;
  }
  return const_cast<connector_anon*>(this);
}



handle
connector_anon::get_read_handle() const
{
//  return handle(m_fds[0]);
	return handle(); // FIXME
}



handle
connector_anon::get_write_handle() const
{
//  return handle(m_fds[1]);
	return handle(); // FIXME
}



error_t
connector_anon::close()
{
  if (!connected()) {
    return ERR_INITIALIZATION;
  }
// FIXME
// FIXME  // We ignore errors from close() here. This is a problem with NFS, as the man
// FIXME  // pages state, but it's the price of the abstraction.
// FIXME  ::close(m_fds[0]);
// FIXME  ::close(m_fds[1]);
// FIXME
// FIXME  m_fds[0] = m_fds[1] = -1;

  return ERR_SUCCESS;
}



error_t
connector_anon::set_blocking_mode(bool state)
{
// FIXME  error_t err = ::packeteer::set_blocking_mode(m_fds[0], state);
// FIXME  if (err != ERR_SUCCESS) {
// FIXME    return err;
// FIXME  }
// FIXME  return ::packeteer::set_blocking_mode(m_fds[1], state);
  return ERR_SUCCESS;
}



error_t
connector_anon::get_blocking_mode(bool & state) const
{
// FIXME  bool states[2] = { false, false };
//  error_t err = ::packeteer::get_blocking_mode(m_fds[0], states[0]);
//  if (err != ERR_SUCCESS) {
//    return err;
//  }
//  err = ::packeteer::get_blocking_mode(m_fds[1], states[1]);
//  if (err != ERR_SUCCESS) {
//    return err;
//  }
//
//  if (states[0] != states[1]) {
//    LOG("The two file descriptors had differing blocking modes.");
//    return ERR_UNEXPECTED;
//  }
//
//  state = states[0];
  return ERR_SUCCESS;
}


}} // namespace packeteer::detail
