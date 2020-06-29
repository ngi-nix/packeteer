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

#include "anon.h"

#include "../../macros.h"
#include "../../win32/sys_handle.h"

#include "pipe_operations.h"
#include "io_operations.h"

#include <packeteer/handle.h>


namespace packeteer::detail {

connector_anon::connector_anon(connector_options const & options)
  : connector_common(CO_STREAM | (options & CO_BLOCKING))
  , m_handles{{}, {}}
{
}



connector_anon::~connector_anon()
{
  connector_anon::close();
}



error_t
connector_anon::create_pipe()
{
  if (connected()) {
    return ERR_INITIALIZATION;
  }

  // Generate a unique address for the pipe
  auto addr = detail::create_anonymous_pipe_name("packeteer-anonymous");
  DLOG("Anonymous pipe address is: " << addr);

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
    EXC_LOG("Could not create anonymous pipe", ex);
    return ex.code();
  } catch (std::exception const & ex) {
    EXC_LOG("Could not create anonymous pipe", ex);
    return ERR_ABORTED;
  } catch (...) {
    ELOG("Could not create anonymous pipe due to an unknown error.");
    return ERR_ABORTED;
  }

  // Now connect the client side.
  handle client;
  auto err = detail::connect_to_pipe(client, addr, is_blocking(),
      true, // Readable,
      false // Non-writable
  );
  switch (err) {
    case ERR_SUCCESS:
    case ERR_ASYNC:
      // Both good
      break;

    default:
      ET_LOG("Could not connect to anonymous pipe", err);
      return err;
  }

  // We poll for a connection, in a loop - this can block.
  bool loop = true;
  do {
    err = detail::poll_for_connection(server);
    switch (err) {
      case ERR_SUCCESS:
        loop = false;
        break;

      case ERR_REPEAT_ACTION:
        continue;

      default:
        ET_LOG("Unknown error when trying to poll for a connection.", err);
        DisconnectNamedPipe(server.sys_handle()->handle);
        CloseHandle(server.sys_handle()->handle);
        return ERR_ABORTED;
    }
  } while (loop);

  // All good - keep the handles!
  m_handles[0] = client;
  m_handles[1] = server;
  m_addr = addr;

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

  CloseHandle(m_handles[0].sys_handle()->handle);
  CloseHandle(m_handles[1].sys_handle()->handle);

  m_handles[0] = m_handles[1] = handle{};

  return ERR_SUCCESS;
}



bool
connector_anon::is_blocking() const
{
  return m_options & CO_BLOCKING;
}


error_t
connector_anon::receive(void * buf, size_t bufsize, size_t & bytes_read,
      ::packeteer::net::socket_address & sender)
{
  // Receive is like read, but we copy the sender address. With anonymous pipes,
  // sender and receiver have identical addresses.
  if (!connected() && !listening()) {
    return ERR_INITIALIZATION;
  }

  ssize_t have_read = -1;
  auto err = detail::read(get_read_handle(), buf, bufsize, have_read);
  if (ERR_SUCCESS == err) {
    bytes_read = have_read;
    sender = net::socket_address{m_addr};
  }
  return err;
}



error_t
connector_anon::send(void const * buf, size_t bufsize, size_t & bytes_written,
      ::packeteer::net::socket_address const & recipient)
{
  // Send is like write - we just don't use the recipient.
  return write(buf, bufsize, bytes_written);
}



size_t
connector_anon::peek() const
{
  return detail::pipe_peek(get_read_handle());
}




} // namespace packeteer::detail
