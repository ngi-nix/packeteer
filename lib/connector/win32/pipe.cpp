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

#include "pipe.h"

#include <packeteer/handle.h>
#include <packeteer/error.h>

#include "../../globals.h"
#include "../../macros.h"
#include "../../win32/sys_handle.h"

#include "pipe_operations.h"
#include "io_operations.h"


namespace packeteer::detail {

namespace {

error_t
create_new_pipe_instance(handle & handle, std::string const & path, bool blocking)
{
  DLOG("Create new " << (blocking ? "blocking" : "non-blocking")
      << " pipe instance at path " << path);

  packeteer::handle h;
  try {
    h = detail::create_named_pipe(path, blocking,
        true,  // Readable
        true,  // Writable
        true   // Remote ok. XXX: we can change this into an option
    );
  } catch (packeteer::exception const & ex) {
    EXC_LOG("Could not create named pipe", ex);
    return ex.code();
  } catch (std::exception const & ex) {
    EXC_LOG("Could not create named pipe", ex);
    return ERR_ABORTED;
  } catch (...) {
    ELOG("Could not create named pipe due to an unknown error.");
    return ERR_ABORTED;
  }

  // Poll for connection. Note that the polling is always asynchronous; it can
  // return success only if a client already started to connect.
  auto err = detail::poll_for_connection(h);
  switch (err) {
    case ERR_SUCCESS:
    case ERR_ASYNC:
      handle = h;
      DLOG("Successfully created new pipe instance!");
      return ERR_SUCCESS;

    default:
      ERRNO_LOG("Unknown error when trying to listen().");

      // Cleanup
      DisconnectNamedPipe(h.sys_handle()->handle);
      CloseHandle(h.sys_handle()->handle);
      return ERR_ABORTED;
  }
}

} // anonymous namespace

connector_pipe::connector_pipe(std::string const & path,
    connector_options const & options)
  : connector_common((options | CO_STREAM) & ~CO_DATAGRAM)
  , m_addr{path}
{
}



connector_pipe::connector_pipe(liberate::net::socket_address const & addr,
    connector_options const & options)
  : connector_common((options | CO_STREAM) & ~CO_DATAGRAM)
  , m_addr{addr}
{
}



connector_pipe::~connector_pipe()
{
  connector_pipe::close();
}



error_t
connector_pipe::connect()
{
  if (connected() || listening()) {
    return ERR_INITIALIZATION;
  }

  auto err = detail::connect_to_pipe(m_handle,
      m_addr.full_str(),
      is_blocking(),
      true, // Readable
      true  // Writable
  );

  switch (err) {
    case ERR_SUCCESS:
    case ERR_ASYNC:
      m_connected = true;
      break;

    default:
      ELOG("Connect failed.");
      break;
  }

  return err;
}



error_t
connector_pipe::listen()
{
  if (connected() || listening()) {
    return ERR_INITIALIZATION;
  }

  error_t err = create_new_pipe_instance(m_handle, m_addr.full_str(),
      is_blocking());

  if (ERR_SUCCESS == err) {
    m_server = true;
  }
  return err;
}



bool
connector_pipe::listening() const
{
  return m_handle.valid() && m_server;
}



bool
connector_pipe::connected() const
{
  return m_handle.valid() && m_connected;
}



connector_interface *
connector_pipe::accept(liberate::net::socket_address & /* unused */)
{
  // There is no need for accept(); we've already got the connection established.
  if (!listening()) {
    ELOG("accept() called, but not listening.");
    return nullptr;
  }

  // Accept is blocking by definition. Calling poll_for_connection() either
  // completes the previously outstanding call, or schedules a new one. Either
  // way, we have to wait here until we have a connection.
  bool loop = true;
  do {
    auto err = detail::poll_for_connection(*const_cast<handle *>(&m_handle));
    switch (err) {
      case ERR_SUCCESS:
        loop = false;
        break;

      case ERR_REPEAT_ACTION:
        continue; // Try again.

      default:
        ET_LOG("Unknown error when trying to accept().", err);

        // Cleanup
        DisconnectNamedPipe(m_handle.sys_handle()->handle);
        CloseHandle(m_handle.sys_handle()->handle);
        return nullptr;
    }
  } while (loop);

  // If we reached here, we've got a connection on our handle. This is great;
  // we need to pass the handle out as the return value. But we *also* have to
  // create a new pipe instance, because the old one will be busy.
  auto ret = new connector_pipe(m_addr, m_options);
  ret->m_handle = m_handle;
  ret->m_server = true;
  ret->m_connected = true;

  // Reset self, then create new pipe instance.
  m_handle = handle{};
  m_server = false;

  error_t err = create_new_pipe_instance(m_handle, m_addr.full_str(),
      is_blocking());
  if (ERR_SUCCESS == err) {
    m_server = true;
  }
  else {
    ET_LOG("Could not create new pipe", err);
  }

  return ret;
}



handle
connector_pipe::get_read_handle() const
{
  return m_handle;
}



handle
connector_pipe::get_write_handle() const
{
  return m_handle;
}



error_t
connector_pipe::close()
{
  if (!listening() && !connected()) {
    return ERR_INITIALIZATION;
  }

  if (m_server) {
    DisconnectNamedPipe(m_handle.sys_handle()->handle);
  }
  CloseHandle(m_handle.sys_handle()->handle);

  m_handle = handle();
  m_server = false;

  return ERR_SUCCESS;
}


bool
connector_pipe::is_blocking() const
{
  return m_options & CO_BLOCKING;
}



error_t
connector_pipe::receive(void * buf, size_t bufsize, size_t & bytes_read,
      liberate::net::socket_address & sender)
{
  // Receive is like read, but we copy the sender address. With pipes,
  // sender and receiver have identical addresses.
  if (!connected() && !listening()) {
    return ERR_INITIALIZATION;
  }

  ssize_t have_read = -1;
  auto err = detail::read(get_read_handle(), buf, bufsize, have_read);
  if (ERR_SUCCESS == err) {
    bytes_read = have_read;
    sender = liberate::net::socket_address{m_addr};
  }
  return err;
}



error_t
connector_pipe::send(void const * buf, size_t bufsize, size_t & bytes_written,
      liberate::net::socket_address const & recipient)
{
  // Send is like write - we just don't use the recipient.
  return write(buf, bufsize, bytes_written);
}




size_t
connector_pipe::peek() const
{
  return detail::pipe_peek(get_read_handle());
}



} // namespace packeteer::detail
