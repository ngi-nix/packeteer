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

  // Name with slash, and escaped slash
  { "foo/bar", "", true },
  { "bar\\/foo", "\\\\.\\pipe\\bar/foo", false },

  // Other errors
  { "foo\\bar", "", true }, // Can't have path components.
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
  auto err = pd::connect_to_pipe(h, "foo", false);

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
  auto err = pd::connect_to_pipe(client, "foo", false);

  ASSERT_EQ(packeteer::ERR_SUCCESS, err);
  ASSERT_TRUE(client.valid());

  // Poll for connection again
  created = pd::poll_for_connection(server);
  EXPECT_EQ(packeteer::ERR_SUCCESS, created);

  CloseHandle(server_sys_handle.handle);
  CloseHandle(client.sys_handle().handle);
}

// TODO:
// - Anonymous pipe functionality - might need different access rights passed to it.
// 
// Basically:
// - create named pipe, N instances
// - whenever an instance connects, it's blocked and will
//   eventually have to be discarded
// - ensure there are walways N free instances by creating new
//   ones.
// - do non-blocking poll_for_connection() so that we can listen
//   on all free N instances
//   - REQUIRES overlapped I/O, always
//   - that is, poll_for_connection needs to always loop on
//     ConnectNamedPipe() if the correct WSAGetLastError is set
//     -> make it a polling function, effectively.
