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

#include "socketpair.h"

#include <packeteer/net/socket_address.h>
#include <packeteer/util/tmp.h>
#include <packeteer/util/path.h>

#include "socket.h"

#include "../../macros.h"

namespace packeteer::detail {

error_t socketpair(int domain, int type, int protocol, SOCKET sockets[2])
{
  // Determine a bind address. For AF_INET and AF_INET6, it can be localhost.
  // For AF_UNIX, we need a temporary but unique-ish path.
  net::socket_address bind_address;
  switch (domain) {
    case AF_INET:
      bind_address = net::socket_address{"127.0.0.1"};
      break;

    case AF_INET6:
      bind_address = net::socket_address{"::1"};
      break;

    case AF_UNIX:
      bind_address = net::socket_address{util::to_posix_path(util::temp_name("packeteer-socketpair-server"))};
      break;

    default:
      return ERR_INVALID_VALUE;
      break;
  }

  // First, create a socket with the given domain, type, proto, etc.
  SOCKET server_sock = INVALID_SOCKET;
  auto err = create_socket(domain, type, protocol, server_sock, true);
  if (err != ERR_SUCCESS) {
    return err;
  }

  // Bind the server socket
  int ret = ::bind(server_sock,
      reinterpret_cast<struct sockaddr const *>(bind_address.buffer()),
      bind_address.bufsize());
  if (ret == SOCKET_ERROR) {
    ERRNO_LOG("Bind failed.");

    close_socket(server_sock);
    return ERR_UNEXPECTED; // TODO translate
  }

  // Determine bound address. This is for random port assignment for IP sockets.
  net::socket_address bound_address;
  int size = bound_address.bufsize_available();
  ret = ::getsockname(server_sock,
      reinterpret_cast<struct sockaddr *>(bound_address.buffer()),
      &size);
  if (ret == SOCKET_ERROR) {
    ERRNO_LOG("Getsockname failed.");

    close_socket(server_sock);
    return ERR_UNEXPECTED; // TODO translate
  }

  // Listen on the server address
  ret = ::listen(server_sock, 1);
  if (ret == SOCKET_ERROR) {
    ERRNO_LOG("Listen failed.");

    close_socket(server_sock);
    return ERR_UNEXPECTED; // TODO translate
  }

  // Create client socket.
  SOCKET client_sock = INVALID_SOCKET;
  err = create_socket(domain, type, protocol, client_sock, false);
  if (err != ERR_SUCCESS) {
    close_socket(server_sock);
    return err;
  }

  // Connect client to server.
  ret = ::connect(client_sock,
      reinterpret_cast<struct sockaddr const *>(bound_address.buffer()),
      bound_address.bufsize());
  if (ret == SOCKET_ERROR) {
    auto winerr = WSAGetLastError();
    if (winerr != WSAEWOULDBLOCK) {
      ERRNO_LOG("Connect failed.");

      close_socket(server_sock);
      close_socket(client_sock);
      return ERR_UNEXPECTED; // TODO translate
    }
  }

  // Accept
  net::detail::address_data buf;
  ::socklen_t len = sizeof(buf);
  SOCKET sock = INVALID_SOCKET;
  while (true) {
    sock = ::accept(server_sock, &buf.sa, &len);
    if (sock != INVALID_SOCKET) {
      break;
    }

    auto wsaerr = WSAGetLastError();

    if (wsaerr == WSAEINTR || wsaerr == WSAEWOULDBLOCK) {
      continue;
    }

    ERR_LOG("Accept failed.", wsaerr);
    close_socket(server_sock);
    close_socket(client_sock);
    return ERR_UNEXPECTED;
  }

  // Cleanup
  close_socket(server_sock);
  set_blocking(client_sock, true);

  // Set results.
  sockets[0] = sock;
  sockets[1] = client_sock;

  return ERR_SUCCESS;
}

} // namespace packeteer::detail
