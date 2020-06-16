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
#include "../../lib/connector/win32/socketpair.h"
#include "../../lib/connector/win32/socket.h"
#include "../../lib/macros.h"

#include <gtest/gtest.h>

#include "../test_name.h"

namespace p7r = packeteer;
namespace pd = p7r::detail;

namespace {

inline void
socketpair_test(int domain)
{
  SOCKET socks[2] = { INVALID_SOCKET, INVALID_SOCKET };

  auto err = pd::socketpair(domain, SOCK_STREAM, 0, socks);
  ASSERT_EQ(p7r::ERR_SUCCESS, err);
  ASSERT_NE(socks[0], INVALID_SOCKET);
  ASSERT_NE(socks[1], INVALID_SOCKET);
  ASSERT_NE((HANDLE) socks[0], INVALID_HANDLE_VALUE);
  ASSERT_NE((HANDLE) socks[1], INVALID_HANDLE_VALUE);

  // Write on client, read on server.
  char buf[] = { 42 };
  DWORD written = 0;
  OVERLAPPED o{};
  BOOL res = WriteFile((HANDLE) socks[1], (void *) buf, sizeof(buf), &written, &o);
  ASSERT_TRUE(res);
  ASSERT_EQ(written, 1);

  buf[0] = 0;
  DWORD read = 0;
  res = ReadFile((HANDLE) socks[0], (void *) buf, sizeof(buf), &read, &o);
  ASSERT_TRUE(res);
  ASSERT_EQ(read, 1);
  ASSERT_EQ(buf[0], 42);

  // Cleanup
  pd::close_socket(socks[0]);
  pd::close_socket(socks[1]);
}


} // anonymous namespace



TEST(SocketPair, create_inet)
{
  socketpair_test(AF_INET);
}


TEST(SocketPair, create_inet6)
{
  socketpair_test(AF_INET6);
}


TEST(SocketPair, create_local)
{
  socketpair_test(AF_UNIX);
}
