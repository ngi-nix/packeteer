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
 * This software is distributed on an "AS IS" BASIS, WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.
 **/
#include "../../lib/connector/win32/pipe_operations.h"

#include <gtest/gtest.h>

#include "../test_name.h"

namespace {

struct test_data
{
  char const * input;
  char const * expected;
  bool error;
} tests[] = {
  // Simple name
  { "foo", "\\\\.\\pipe\\foo", false },

  // Name with slash, and escaped slash, and backslash
  { "foo/bar", "\\\\.\\pipe\\foo/bar", false },
  { "bar\\/foo", "\\\\.\\pipe\\bar/foo", false },
  { "foo\\bar", "\\\\.\\pipe\\foo/bar", false }, // Can't have path components.

  // Preserve pipe prefix
  { "\\\\.\\PiPe\\asdf", "\\\\.\\PiPe\\asdf", false },

  // But convert pipe prefix if it's in slashes
  { "//./PiPe/slashed", "\\\\.\\PiPe\\slashed", false },
  { "//./PiPe/slashed/two", "\\\\.\\PiPe\\slashed/two", false },
  { "//./PiPe/slashed\/two+", "\\\\.\\PiPe\\slashed/two+", false },
  { "//./PiPe/slashed\\three", "\\\\.\\PiPe\\slashed/three", false },

  // Other errors
  { "", "", true }, // Can't have no name.
};


std::string generate_name(testing::TestParamInfo<test_data> const & info)
{
  return symbolize_name(info.param.input);
}

} // anonymous namespace


class NormalizePipePath
  : public testing::TestWithParam<test_data>
{
};


TEST_P(NormalizePipePath, normalize_pipe_path)
{
  namespace pd = packeteer::detail;

  auto td = GetParam();

  if (td.error) {
    ASSERT_THROW(pd::normalize_pipe_path(td.input), packeteer::exception);
  }
  else {
    std::string output = pd::normalize_pipe_path(td.input);

    ASSERT_EQ(std::string{td.expected}, output);
  }
}

INSTANTIATE_TEST_CASE_P(connector, NormalizePipePath, testing::ValuesIn(tests),
    generate_name);



/*****************************************************************************
 * PipeOperations
 */

TEST(PipeOperations, create_named_bad_name)
{
  namespace pd = packeteer::detail;

  ASSERT_THROW(pd::create_named_pipe("", true, true), packeteer::exception);
}


TEST(PipeOperations, create_named_blocking)
{
  namespace pd = packeteer::detail;

  auto res = pd::create_named_pipe("foo", true, false);
  auto sys_handle = res.sys_handle();

  ASSERT_NE(INVALID_HANDLE_VALUE, sys_handle.handle);
  ASSERT_FALSE(sys_handle.overlapped);

  CloseHandle(sys_handle.handle);
}


TEST(PipeOperations, create_named_non_blocking)
{
  namespace pd = packeteer::detail;

  auto res = pd::create_named_pipe("foo", false, false);
  auto sys_handle = res.sys_handle();

  ASSERT_NE(INVALID_HANDLE_VALUE, sys_handle.handle);
  ASSERT_TRUE(sys_handle.overlapped);

  CloseHandle(sys_handle.handle);
}


TEST(PipeOperations, write_writable)
{
  namespace pd = packeteer::detail;

  auto res = pd::create_named_pipe("foo", true, false);
  auto sys_handle = res.sys_handle();

  EXPECT_NE(INVALID_HANDLE_VALUE, sys_handle.handle);
  EXPECT_FALSE(sys_handle.overlapped);

  // Write to the handle.
  auto test = "foo";
  DWORD written = 0;
  auto write_res = WriteFile(sys_handle.handle,
    test, 3, &written, nullptr);

  // Writing should "work" in the sense that we should get an error back that the pipe is listening.
  ASSERT_FALSE(write_res);
  ASSERT_EQ(ERROR_PIPE_LISTENING, WSAGetLastError());

  CloseHandle(sys_handle.handle);
}


TEST(PipeOperations, write_readonly)
{
  namespace pd = packeteer::detail;

  auto res = pd::create_named_pipe("foo", true, true);
  auto sys_handle = res.sys_handle();

  EXPECT_NE(INVALID_HANDLE_VALUE, sys_handle.handle);
  EXPECT_FALSE(sys_handle.overlapped);

  // Write to the handle.
  auto test = "foo";
  DWORD written = 0;
  auto write_res = WriteFile(sys_handle.handle,
    test, 3, &written, nullptr);

  // Writing should not work - it was opened for reading only, so we need to get an error.
  ASSERT_FALSE(write_res);
  ASSERT_EQ(ERROR_ACCESS_DENIED, WSAGetLastError());

  CloseHandle(sys_handle.handle);
}


TEST(PipeOperations, poll_for_connection)
{
  namespace pd = packeteer::detail;

  // Make non-blocking, because the Windows APIs want you to connect from the
  // client before connecting to the server.
  auto res = pd::create_named_pipe("foo", false, false);
  auto sys_handle = res.sys_handle();

  EXPECT_NE(INVALID_HANDLE_VALUE, sys_handle.handle);
  EXPECT_TRUE(sys_handle.overlapped);

  auto created = pd::poll_for_connection(res);
  ASSERT_EQ(packeteer::ERR_REPEAT_ACTION, created);

  CloseHandle(sys_handle.handle);
}


TEST(PipeOperations, open_nonexistent_pipe)
{
  namespace pd = packeteer::detail;

  packeteer::handle h;
  auto err = pd::connect_to_pipe(h, "foo", false, false);

  ASSERT_EQ(packeteer::ERR_FS_ERROR, err);
  ASSERT_FALSE(h.valid());
}


TEST(PipeOperations, open_pipe)
{
  namespace pd = packeteer::detail;

  // Make non-blocking, because the Windows APIs want you to connect from the
  // client before connecting to the server.
  auto server = pd::create_named_pipe("foo", false, false);
  auto server_sys_handle = server.sys_handle();

  EXPECT_NE(INVALID_HANDLE_VALUE, server_sys_handle.handle);
  EXPECT_TRUE(server_sys_handle.overlapped);

  auto created = pd::poll_for_connection(server);
  EXPECT_EQ(packeteer::ERR_REPEAT_ACTION, created);

  // Client
  packeteer::handle client;
  auto err = pd::connect_to_pipe(client, "foo", false, false);

  ASSERT_EQ(packeteer::ERR_SUCCESS, err);
  ASSERT_TRUE(client.valid());

  // Poll for connection again
  created = pd::poll_for_connection(server);
  ASSERT_EQ(packeteer::ERR_SUCCESS, created);

  CloseHandle(server_sys_handle.handle);
  CloseHandle(client.sys_handle().handle);
}



TEST(PipeOperations, open_pipe_multiple_clients_fail)
{
  namespace pd = packeteer::detail;

  // Make non-blocking, because the Windows APIs want you to connect from the
  // client before connecting to the server.
  auto server = pd::create_named_pipe("foo", false, false);
  auto server_sys_handle = server.sys_handle();

  EXPECT_NE(INVALID_HANDLE_VALUE, server_sys_handle.handle);
  EXPECT_TRUE(server_sys_handle.overlapped);

  auto created = pd::poll_for_connection(server);
  EXPECT_EQ(packeteer::ERR_REPEAT_ACTION, created);

  // Client #1
  packeteer::handle client1;
  auto err = pd::connect_to_pipe(client1, "foo", false, false);

  EXPECT_EQ(packeteer::ERR_SUCCESS, err);
  EXPECT_TRUE(client1.valid());

  // Poll for connection again
  created = pd::poll_for_connection(server);
  EXPECT_EQ(packeteer::ERR_SUCCESS, created);

  // Client #2
  packeteer::handle client2;
  err = pd::connect_to_pipe(client2, "foo", false, false);

  ASSERT_EQ(packeteer::ERR_REPEAT_ACTION, err);
  ASSERT_FALSE(client2.valid());


  CloseHandle(server_sys_handle.handle);
  CloseHandle(client1.sys_handle().handle);
  CloseHandle(client2.sys_handle().handle);
}


TEST(PipeOperations, messaging)
{
  namespace pd = packeteer::detail;

  // Make non-blocking, because the Windows APIs want you to connect from the
  // client before connecting to the server.
  auto server = pd::create_named_pipe("foo", false, false);
  auto server_sys_handle = server.sys_handle();

  EXPECT_NE(INVALID_HANDLE_VALUE, server_sys_handle.handle);
  EXPECT_TRUE(server_sys_handle.overlapped);

  auto created = pd::poll_for_connection(server);
  EXPECT_EQ(packeteer::ERR_REPEAT_ACTION, created);

  // Client #1
  packeteer::handle client1;
  auto err = pd::connect_to_pipe(client1, "foo", false, false);

  EXPECT_EQ(packeteer::ERR_SUCCESS, err);
  EXPECT_TRUE(client1.valid());

  // Write server
  auto test = "foo";
  DWORD written = 0;
  auto write_res = WriteFile(server.sys_handle().handle,
    test, 3, &written, nullptr);

  ASSERT_TRUE(write_res);
  ASSERT_EQ(written, 3);

  // Read client
  char buf[200] = { 0 };
  DWORD read = 0;
  auto read_res = ReadFile(client1.sys_handle().handle,
      buf, sizeof(buf), &read, nullptr);

  ASSERT_TRUE(read_res);
  ASSERT_EQ(read, 3);

  for (int i = 0 ; i < read ; ++i) {
    ASSERT_EQ(test[i], buf[i]);
  }

  // Write client
  test = "bar";
  written = 0;
  write_res = WriteFile(client1.sys_handle().handle,
    test, 3, &written, nullptr);

  ASSERT_TRUE(write_res);
  ASSERT_EQ(written, 3);

  // Read server
  read = 0;
  read_res = ReadFile(server.sys_handle().handle,
      buf, sizeof(buf), &read, nullptr);

  ASSERT_TRUE(read_res);
  ASSERT_EQ(read, 3);

  for (int i = 0 ; i < read ; ++i) {
    ASSERT_EQ(test[i], buf[i]);
  }


  CloseHandle(server_sys_handle.handle);
  CloseHandle(client1.sys_handle().handle);
}



TEST(PipeOperations, anonymous_pipe_name)
{
  namespace pd = packeteer::detail;

  auto name1 = pd::create_anonymous_pipe_name();
  ASSERT_FALSE(name1.empty());

  // Check that it's canonical
  auto normalized = pd::normalize_pipe_path(name1);
  ASSERT_EQ(name1, normalized);

  // Check that multiple calls never yield the same name.
  std::string names[10];
  for (int i = 0 ; i < 10 ; ++i) {
    names[i] = pd::create_anonymous_pipe_name();
  }

  // Check all against each other.
  for (int i = 0 ; i < 10 ; ++i) {
    for (int j = 0 ; j < 10 ; ++j) {
      if (i == j) continue; // No need to compare to self
      ASSERT_NE(name1, names[i]);
      ASSERT_NE(names[i], names[j]);
    }
  }
}
