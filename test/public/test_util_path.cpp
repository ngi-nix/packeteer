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
#include <packeteer/util/path.h>

#include <gtest/gtest.h>

#include "../test_name.h"

namespace {

struct test_data_from_win32
{
  std::string name;
  std::string canonical_posix;
  std::string input_win32;
} tests_from_win32[] = {
  { "drive letters",
    "/c", "C:", },
  { "drive letters with delim",
    "/c", "C:\\", },

  { "root paths",
    "/foo", "\\foo", },
  { "root paths with delim",
    "/foo", "\\foo\\", },

  { "longer path",
    "/c/tmp/foo", "C:\\tmp\\foo", },
  { "longer path with delim",
    "/c/tmp/foo", "C:\\tmp\\foo\\", },

  { "quoted delimiter",
    "/c/tmp\/foo", "C:\\tmp\/foo", },
};


struct test_data_from_posix
{
  std::string name;
  std::string canonical_win32;
  std::string input_posix;
} tests_from_posix[] = {
  { "drive letters",
    "C:", "/c", },
  { "drive letters with delim",
    "C:", "/c/", },

  { "root paths",
    "\\foo", "/foo", },
  { "root paths with delim",
    "\\foo", "/foo/", },

  { "longer path",
    "C:\\tmp\\foo", "/c/tmp/foo", },
  { "longer path with delim",
    "C:\\tmp\\foo", "/c/tmp/foo/", },

  { "quoted delimiter",
    "C:\\tmp\\\\foo", "/c/tmp\\foo", },
};


template <typename T>
std::string generate_name(testing::TestParamInfo<T> const & info)
{
  return symbolize_name(info.param.name);
}

} // anonymous namespace


class UtilPathFromWin32
  : public testing::TestWithParam<test_data_from_win32>
{
};



TEST_P(UtilPathFromWin32, convert)
{
  auto td = GetParam();

  auto posix = packeteer::util::to_posix_path(td.input_win32);
  ASSERT_EQ(td.canonical_posix, posix);
}


INSTANTIATE_TEST_SUITE_P(util, UtilPathFromWin32, testing::ValuesIn(tests_from_win32),
    generate_name<test_data_from_win32>);


class UtilPathFromPosix
  : public testing::TestWithParam<test_data_from_posix>
{
};


TEST_P(UtilPathFromPosix, convert)
{
  auto td = GetParam();

  auto win32 = packeteer::util::to_win32_path(td.input_posix);
  ASSERT_EQ(td.canonical_win32, win32);
}


INSTANTIATE_TEST_SUITE_P(util, UtilPathFromPosix, testing::ValuesIn(tests_from_posix),
    generate_name<test_data_from_posix>);

