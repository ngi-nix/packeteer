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
  ASSERT_EQ(ERROR_PIPE_LISTENING, GetLastError());

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
  ASSERT_EQ(ERROR_ACCESS_DENIED, GetLastError());

  CloseHandle(sys_handle.handle);
}
