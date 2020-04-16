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
 * This software is distributed on an "AS IS" BASIS, WITHOUT ANY WARRANTY
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.
 **/
#include <build-config.h>

#include "../anon.h"

#include "../../macros.h"
#include "../../connector/win32/pipe_operations.h"
#include "../../win32/sys_handle.h"

#include <packeteer/handle.h>


namespace packeteer::detail {

connector_anon::connector_anon(connector_options const & options)
  : connector_interface(CO_STREAM | (options & CO_BLOCKING))
  , m_handles{{}, {}}
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

  // TODO: a single read/write handle is not sufficient for transmitting
  //       data from the write end to the read end.
  //       - Need a write-only handle
  //       - also a read-only handle
  //       - connect the two
  //       - pipe_operations.h may not be entirely sufficient for this
  //         (only has a bool readonly flag)

  // Generate a unique address for the pipe
  auto addr = detail::create_anonymous_pipe_name("packeteer-anonymous");
  LOG("Anonymous pipe address is: " << addr);

  // It doesn't really matter whether we make the pipe server the read or
  // the write handle. Arbitrarily we write from the pipe server to the pipe
  // client.
  handle server;
  try {
    server = detail::create_named_pipe(addr, is_blocking(),
        false, // Non-readable
        true,  // Writable
        false  // No remote connections
    );
  } catch (packeteer::exception const & ex) {
    ERR_LOG("Could not create anonymous pipe", ex);
    return ex.code();
  } catch (std::exception const & ex) {
    ERR_LOG("Could not create anonymous pipe", ex);
    return ERR_ABORTED;
  } catch (...) {
    LOG("Could not create anonymous pipe due to an unknown error.");
    return ERR_ABORTED;
  }

  // We poll for a connection, and expect that the result is ERR_REPEAT_ACTION
  auto err = detail::poll_for_connection(server);
  if (ERR_REPEAT_ACTION != err) {
    ERRNO_LOG("Unknown error when trying to poll for a connection.");
    DisconnectNamedPipe(server.sys_handle()->handle);
    CloseHandle(server.sys_handle()->handle);
    return ERR_ABORTED;
  }

  // Now connect the client side.
  handle client;
  err = detail::connect_to_pipe(client, addr, is_blocking(),
      true, // Readable,
      false // Non-writable
  );
  if (ERR_SUCCESS != err) {
    ERRNO_LOG("Could not connect to anonymous pipe");
    return err;
  }

  // All good - keep the handles!
  m_handles[0] = client;
  m_handles[1] = server;

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
	// FIXME
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
// FIXME
// FIXME  // We ignore errors from close() here. This is a problem with NFS, as the man
// FIXME  // pages state, but it's the price of the abstraction.
// FIXME  ::close(m_fds[0]);
// FIXME  ::close(m_fds[1]);
// FIXME
// FIXME  m_fds[0] = m_fds[1] = -1;

  return ERR_SUCCESS;
}



bool
connector_anon::is_blocking() const
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
  return false;
}


} // namespace packeteer::detail
