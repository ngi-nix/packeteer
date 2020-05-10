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

#include <packeteer/net/socket_address.h>
#include <packeteer/handle.h>
#include <packeteer/error.h>

#include "../../globals.h"
#include "../../macros.h"

#include "../../win32/sys_handle.h"


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

    case WSAEADDRNOTAVAIL:  // Technically, ports
      return ERR_ADDRESS_NOT_AVAILABLE; // FIXME check posix

    case WSAENOBUFS:
      return ERR_OUT_OF_MEMORY;

    case WSAENOTSOCK:
    case WSAENOPROTOOPT:
    case WSAEOPNOTSUPP:
      return ERR_UNSUPPORTED_ACTION;

    case WSAENETRESET:
    case WSAENOTCONN:
    case WSAENETDOWN:
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

#if 0 // FIXME AF_LOCAL
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
  SOCKET sock = WSASocket(domain, type, proto, NULL, 0, WSA_FLAG_OVERLAPPED);
  if (sock == INVALID_SOCKET) {
    ERRNO_LOG("create_socket socket failed!");
    return translate_system_error(WSAGetLastError());
  }

  // Set socket to close forcibly.
  ::linger option;
  option.l_onoff = 1;
  option.l_linger = 0;
  int ret = ::setsockopt(sock, SOL_SOCKET, SO_LINGER,
      reinterpret_cast<char *>(&option), sizeof(option));
  if (ret == SOCKET_ERROR) {
    auto err = WSAGetLastError();

    closesocket(sock);
    h = handle::INVALID_SYS_HANDLE;

    ERR_LOG("create_socket setsockopt failed!", err);
    return translate_system_error(err);
  }

  // Result
  h = std::make_shared<handle::opaque_handle>(
      reinterpret_cast<HANDLE>(sock));
  h->blocking = blocking;

  return ERR_SUCCESS;
}


} // anonymous namespace



connector_socket::connector_socket(net::socket_address const & addr,
    connector_options const & options)
  : connector_interface(options)
  , m_addr(addr)
{
}



connector_socket::connector_socket(connector_options const & options)
  : connector_interface(options)
{
}



error_t
connector_socket::socket_connect(int domain, int type, int proto)
{
  if (connected() || listening()) {
    return ERR_INITIALIZATION;
  }

  // First, create handle
  handle::sys_handle_t h = handle::INVALID_SYS_HANDLE;
  error_t err = create_socket_handle(domain, type, proto, h,
      m_options & CO_BLOCKING);
  if (h == handle::INVALID_SYS_HANDLE) {
    return err;
  }

  // FIXME posix docs
  // Now try to connect the socket with the address
  while (true) {
    int ret = ::connect(h->socket,
        reinterpret_cast<struct sockaddr const *>(m_addr.buffer()),
        m_addr.bufsize());
    auto err = WSAGetLastError();
    ERR_LOG("XXX ", err); // FIXME

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
    if (err == WSAEINPROGRESS || err == WSAEALREADY) {
      m_handle = h;
      m_server = false;
      m_connected = true;
      return ERR_ASYNC;
    }

    // Singal interrupt, try again.
    if (err == WSAEINTR) {
      continue;
    }

    // Otherwise we have an error.
    closesocket(h->socket);

    ERR_LOG("connector_socket connect failed!", err);
    return translate_system_error(err);
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

  // First, create handle
  h = handle::INVALID_SYS_HANDLE;
  error_t err = create_socket_handle(domain, type, proto, h,
      m_options & CO_BLOCKING);
  if (h == handle::INVALID_SYS_HANDLE) {
    return err;
  }

  // Now try to bind the socket to the address
  int ret = ::bind(h->socket,
      reinterpret_cast<struct sockaddr const *>(m_addr.buffer()),
      m_addr.bufsize());
  if (ret == SOCKET_ERROR) {
    auto err = WSAGetLastError();

    closesocket(h->socket);
    h = handle::INVALID_SYS_HANDLE;

    ERR_LOG("connector_socket bind failed; address is: "
        << m_addr.full_str(), err);
    return translate_system_error(err);
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
    closesocket(h->socket);
    h = handle::INVALID_SYS_HANDLE;

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
  closesocket(m_handle->socket);

  m_handle = handle::INVALID_SYS_HANDLE;
  m_server = false;
  m_connected = false; // FIXME posix

  return ERR_SUCCESS;
}



error_t
connector_socket::socket_accept(handle::sys_handle_t & new_handle, net::socket_address & addr)
{
  // There is no need for accept(); we've already got the connection established.
  if (!listening()) {
    return ERR_INITIALIZATION;
  }

  // Accept connection
  new_handle = handle::INVALID_SYS_HANDLE;
  net::detail::address_data buf;
  ::socklen_t len = sizeof(buf);

  SOCKET sock = INVALID_SOCKET;
  while (true) {
    sock = ::accept(m_handle->socket, &buf.sa, &len);
    if (sock != INVALID_SOCKET) {
      break;
    }

    auto err = WSAGetLastError();

    // Signal interrupt, try again.
    if (err == WSAEINTR) {
      continue;
    }

    ERR_LOG("connector_socket accept failed!", err);
    return translate_system_error(err);
  }

  // Result
  new_handle = std::make_shared<handle::opaque_handle>(
      reinterpret_cast<HANDLE>(sock));
  new_handle->blocking = m_options & CO_BLOCKING;

  // Keep address and return success.
  addr = net::socket_address(&buf, len);
  return ERR_SUCCESS;
}



bool
connector_socket::is_blocking() const
{
  return m_handle != handle::INVALID_SYS_HANDLE && m_handle->blocking;
}


} // namespace packeteer::detail