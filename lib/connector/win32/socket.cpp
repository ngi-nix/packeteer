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
 * This software is distributed on an "AS IS" BASIS, WITHOUT ANY WARRANTY
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.
 **/
#include <build-config.h>

#include "socket.h"

#include <packeteer/handle.h>
#include <packeteer/error.h>

#include "../../globals.h"
#include "../../macros.h"

#include "../../win32/sys_handle.h"

#include "io_operations.h"


namespace packeteer::detail {

namespace {

inline error_t
translate_system_error(int err)
{
  switch (err) {
    case WSANOTINITIALISED:
    case WSAEPROVIDERFAILEDINIT:
      return ERR_INITIALIZATION;

    case WSAEAFNOSUPPORT:
    case WSAEPROTONOSUPPORT:
    case WSAEPROTOTYPE:
    case WSAESOCKTNOSUPPORT:
      return ERR_INVALID_OPTION;

    case WSAEINVAL:
    case WSAEFAULT:
      return ERR_INVALID_VALUE;

    case WSAEMFILE:
      return ERR_NUM_FILES;

    case WSAEADDRNOTAVAIL:
      return ERR_NUM_FILES; // technically, ports.

    case WSAENOBUFS:
      return ERR_OUT_OF_MEMORY;

    case WSAENOTSOCK:
    case WSAENOPROTOOPT:
    case WSAEOPNOTSUPP:
      return ERR_UNSUPPORTED_ACTION;

    case WSAENOTCONN:
    case WSAENETDOWN:
      return ERR_NO_CONNECTION;

    case WSAENETRESET:
    case WSAEHOSTUNREACH:
      return ERR_NETWORK_UNREACHABLE;

    case WSAECONNREFUSED:
      return ERR_CONNECTION_REFUSED;

    case WSAECONNRESET:
      return ERR_CONNECTION_ABORTED;
 
    case WSAEACCES:
      return ERR_ACCESS_VIOLATION;

    case WSAEADDRINUSE:
      return ERR_ADDRESS_IN_USE;

    case WSAETIMEDOUT:
      return ERR_TIMEOUT;

    case WSAEISCONN:
      return ERR_INITIALIZATION;

    // XXX on POSIX for AF_LOCAL
    // There is no documented Win32 equivalent, but it surely exists.
#if 0
    case ENOENT:
    case ENOTDIR:
    case EROFS:
        return ERR_FS_ERROR;
#endif

    case WSAEINPROGRESS:
    case WSAEWOULDBLOCK:
    case WSAEINVALIDPROVIDER:
    case WSAEINVALIDPROCTABLE:
    default:
      return ERR_UNEXPECTED;
  }
}



error_t
create_socket_handle(int domain, int type, int proto,
    handle::sys_handle_t & h,
    bool blocking)
{
  DLOG("create_socket_handle(" << blocking << ")");
  SOCKET sock = INVALID_SOCKET;
  auto err = create_socket(domain, type, proto, sock,
      true); // We want blocking handles and OVERLAPPED here.
  if (err != ERR_SUCCESS) {
    return err;
  }

  // Result
  h = std::make_shared<handle::opaque_handle>(
      reinterpret_cast<HANDLE>(sock));
  h->blocking = blocking;

  return ERR_SUCCESS;
}


} // anonymous namespace


error_t
create_socket(int domain, int type, int proto, SOCKET & socket,
    bool blocking)
{
  SOCKET sock = WSASocket(domain, type, proto, NULL, 0, WSA_FLAG_OVERLAPPED);
  if (sock == INVALID_SOCKET) {
    ERRNO_LOG("create_socket socket failed!");
    return translate_system_error(WSAGetLastError());
  }

  // Set socket to close forcibly.
  if (SOCK_STREAM == type) {
    ::linger option;
    option.l_onoff = TRUE;
    option.l_linger = 0;
    int ret = ::setsockopt(sock, SOL_SOCKET, SO_LINGER,
        reinterpret_cast<char *>(&option), sizeof(option));
    if (ret == SOCKET_ERROR) {
      auto err = WSAGetLastError();

      close_socket(sock);
      socket = INVALID_SOCKET;

      ERR_LOG("create_socket setsockopt failed!", err);
      return translate_system_error(err);
    }
  }

  // Set security flag - prevent other processes from binding to the same
  // port. This does not make sense for AF_UNIX, as it'll seem to prevent
  // binding the client to an address the server is listening to.
  if (domain != AF_UNIX) {
    int exclusive = 1;
    int ret = ::setsockopt(sock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
        reinterpret_cast<char *>(&exclusive), sizeof(exclusive));
    if (ret == SOCKET_ERROR) {
      auto err = WSAGetLastError();

      close_socket(sock);
      socket = INVALID_SOCKET;

      ERR_LOG("create_socket setsockopt failed!", err);
      return translate_system_error(err);
    }
  }

  // Make non-blocking.
  auto err = set_blocking(sock, blocking);
  if (ERR_SUCCESS != err) {
    close_socket(sock);
    socket = INVALID_SOCKET;
    return err;
  }

  // Result
  socket = sock;
  return ERR_SUCCESS;
}



error_t
set_blocking(SOCKET sock, bool blocking)
{
  u_long block_mode = blocking ? 0 : 1; // yes, check the docks.
  int ret = ::ioctlsocket(sock, FIONBIO, &block_mode);
  if (ret == SOCKET_ERROR) {
    auto err = WSAGetLastError();

    ERR_LOG("create_socket ioctlsocket failed!", err);
    return translate_system_error(err);
  }
  return ERR_SUCCESS;
}



void
close_socket(SOCKET sock)
{
  shutdown(sock, SD_BOTH);
  closesocket(sock);
}





connector_socket::connector_socket(peer_address const & addr,
    connector_options const & options)
  : connector_common{addr, options}
{
}



error_t
connector_socket::socket_connect(int domain, int type, int proto)
{
  if (connected() || listening()) {
    return ERR_INITIALIZATION;
  }

  // https://gitlab.com/interpeer/packeteer/-/issues/18
  if (m_address.socket_address().type() == liberate::net::AT_UNSPEC) {
    ELOG("Unnamed CT_LOCAL connectors are not supported yet.");
    return ERR_INVALID_VALUE;
  }

  // First, create handle
  handle::sys_handle_t h = handle::INVALID_SYS_HANDLE;
  error_t err = create_socket_handle(domain, type, proto, h,
      m_options & CO_BLOCKING);
  if (h == handle::INVALID_SYS_HANDLE) {
    return err;
  }

  // Now try to connect the socket with the address
  while (true) {
    int ret = ::connect(h->socket,
        reinterpret_cast<struct sockaddr const *>(m_address.socket_address().buffer()),
        m_address.socket_address().bufsize());
    auto wsaerr = WSAGetLastError();

    if (ret != SOCKET_ERROR) {
      // Finally, set the handle
      m_handle = h;
      m_server = false;
      m_connected = true;

      // Simulate non-blocking mode, also for socket types that return
      // success. This helps the calling code treat all sockets the same.
      if (m_options & CO_NON_BLOCKING) {
        return ERR_ASYNC;
      }
      return ERR_SUCCESS;
    }

    // We have a non-blocking socket, and connection will take a while to
    // complete. We'll treat this as success, but have to return ERR_ASYNC.
    if (wsaerr == WSAEINPROGRESS || wsaerr == WSAEALREADY) {
      m_handle = h;
      m_server = false;
      m_connected = true;
      return ERR_ASYNC;
    }

    // Singal interrupt, try again.
    if (wsaerr == WSAEINTR) {
      continue;
    }

    // Otherwise we have an error.
    close_socket(h->socket);

    ERR_LOG("connector_socket connect failed!", wsaerr);
    return translate_system_error(wsaerr);
  }
  PACKETEER_FLOW_CONTROL_GUARD;
}



error_t
connector_socket::socket_create(int domain, int type, int proto, handle::sys_handle_t & h)
{
  if (connected() || listening()) {
    return ERR_INITIALIZATION;
  }

  h = handle::INVALID_SYS_HANDLE;
  error_t err = create_socket_handle(domain, type, proto, h,
      m_options & CO_BLOCKING);
  if (h == handle::INVALID_SYS_HANDLE) {
    return err;
  }
  return ERR_SUCCESS;
}



error_t
connector_socket::socket_bind(int domain, int type, int proto, handle::sys_handle_t & h)
{
  if (connected() || listening()) {
    return ERR_INITIALIZATION;
  }

  // https://gitlab.com/interpeer/packeteer/-/issues/18
  if (m_address.socket_address().type() == liberate::net::AT_UNSPEC) {
    ELOG("Unnamed CT_LOCAL connectors are not supported yet.");
    return ERR_INVALID_VALUE;
  }

  // First, create handle
  h = handle::INVALID_SYS_HANDLE;
  error_t err = create_socket_handle(domain, type, proto, h,
      m_options & CO_BLOCKING);
  if (h == handle::INVALID_SYS_HANDLE) {
    return err;
  }

  // Now try to bind the socket to the address
  int ret = ::bind(h->socket,
      reinterpret_cast<struct sockaddr const *>(m_address.socket_address().buffer()),
      m_address.socket_address().bufsize());
  if (ret == SOCKET_ERROR) {
    auto wsaerr = WSAGetLastError();

    close_socket(h->socket);
    h = handle::INVALID_SYS_HANDLE;

    ERR_LOG("connector_socket bind failed; address is: "
        << m_address.socket_address().full_str(), wsaerr);
    return translate_system_error(wsaerr);
  }
  return ERR_SUCCESS;
}


error_t
connector_socket::socket_listen(handle::sys_handle_t h)
{
  if (connected() || listening()) {
    return ERR_INITIALIZATION;
  }

  // Turn the socket into a listening socket.
  int ret = ::listen(h->socket, PACKETEER_LISTEN_BACKLOG);
  if (ret == SOCKET_ERROR) {
    auto err = WSAGetLastError();
    close_socket(h->socket);

    ERR_LOG("connector_socket listen failed!", err);
    return translate_system_error(err);
  }
  return ERR_SUCCESS;
}




bool
connector_socket::listening() const
{
  return m_handle != handle::INVALID_SYS_HANDLE && m_server;
}



bool
connector_socket::connected() const
{
  return m_handle != handle::INVALID_SYS_HANDLE && m_connected;
}



handle
connector_socket::get_read_handle() const
{
  return handle{m_handle};
}



handle
connector_socket::get_write_handle() const
{
  return handle{m_handle};
}



error_t
connector_socket::socket_close()
{
  if (!listening() && !connected()) {
    return ERR_INITIALIZATION;
  }

  // We ignore errors from close() here.
  // For local sockets, there is a problem with NFS as the man pages state, but
  // it's the price of the abstraction.
  close_socket(m_handle->socket);

  m_handle = handle::INVALID_SYS_HANDLE;
  m_server = false;
  m_connected = false;

  return ERR_SUCCESS;
}



error_t
connector_socket::socket_accept(handle::sys_handle_t & new_handle, liberate::net::socket_address & addr)
{
  // There is no need for accept(); we've already got the connection established.
  if (!listening()) {
    return ERR_INITIALIZATION;
  }

  // Accept connection
  new_handle = handle::INVALID_SYS_HANDLE;
  liberate::net::detail::address_data buf;
  ::socklen_t len = sizeof(buf);

  SOCKET sock = INVALID_SOCKET;
  while (true) {
    sock = ::accept(m_handle->socket, reinterpret_cast<sockaddr *>(&buf), &len);
    if (sock != INVALID_SOCKET) {
      break;
    }

    auto err = WSAGetLastError();

    // Signal interrupt, try again.
    if (err == WSAEINTR) {
      continue;
    }

    if (err == WSAEWOULDBLOCK) {
      // See comment in posix/socket.cpp
      return ERR_REPEAT_ACTION;
    }

    ERR_LOG("connector_socket accept failed!", err);
    return translate_system_error(err);
  }

  // Result
  new_handle = std::make_shared<handle::opaque_handle>(
      reinterpret_cast<HANDLE>(sock));
  new_handle->blocking = m_options & CO_BLOCKING;

  // Keep address and return success.
  addr = liberate::net::socket_address(&buf, len);
  return ERR_SUCCESS;
}



bool
connector_socket::is_blocking() const
{
  return m_handle != handle::INVALID_SYS_HANDLE && m_handle->blocking;
}



error_t
connector_socket::receive(void * buf, size_t bufsize, size_t & bytes_read,
      liberate::net::socket_address & sender)
{
  if (!connected() && !listening()) {
    return ERR_INITIALIZATION;
  }

  ssize_t have_read = -1;
  liberate::net::socket_address addr;
  auto err = detail::receive(get_read_handle(), buf, bufsize, have_read, addr);
  if (ERR_SUCCESS == err) {
    bytes_read = have_read;
    sender = addr;
  }
  return err;
}



error_t
connector_socket::send(void const * buf, size_t bufsize, size_t & bytes_written,
      liberate::net::socket_address const & recipient)
{
  if (!connected() && !listening()) {
    return ERR_INITIALIZATION;
  }

  ssize_t written = -1;
  auto err = detail::send(get_write_handle(), buf, bufsize, written,
      recipient);
  if (ERR_SUCCESS == err) {
    bytes_written = written;
  }
  return err;
}



size_t
connector_socket::peek() const
{
  return detail::socket_peek(get_read_handle());
}


} // namespace packeteer::detail
