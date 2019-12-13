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
#include "../../lib/util/string.h"

#include <gtest/gtest.h>


TEST(UtilString, to_lower)
{
  namespace pu = packeteer::util;

  ASSERT_EQ("foo", pu::to_lower("foo"));
  ASSERT_EQ("foo", pu::to_lower("Foo"));
  ASSERT_EQ("foo", pu::to_lower("fOo"));
  ASSERT_EQ("foo", pu::to_lower("foO"));
  ASSERT_EQ("foo", pu::to_lower("FOO"));

  ASSERT_EQ("", pu::to_lower(""));
  ASSERT_EQ("a", pu::to_lower("A"));
}


TEST(UtilString, to_upper)
{
  namespace pu = packeteer::util;

  ASSERT_EQ("FOO", pu::to_upper("foo"));
  ASSERT_EQ("FOO", pu::to_upper("Foo"));
  ASSERT_EQ("FOO", pu::to_upper("fOo"));
  ASSERT_EQ("FOO", pu::to_upper("foO"));
  ASSERT_EQ("FOO", pu::to_upper("FOO"));

  ASSERT_EQ("", pu::to_upper(""));
  ASSERT_EQ("A", pu::to_upper("a"));
}


TEST(UtilString, case_insensitive_search)
{
  namespace pu = packeteer::util;

  auto res = pu::ifind("This is a Test String", "test");
  ASSERT_GE(res, 0); // Less than zero would be failure

  // "test" is at offset 10
  ASSERT_EQ(10, res);

  // Find something at the beginning and end of strings
  ASSERT_EQ(0, pu::ifind("foobar", "FOO"));
  ASSERT_EQ(3, pu::ifind("foobar", "Bar"));

  // Return -1 if string can't be found
  ASSERT_EQ(-1, pu::ifind("foobar", "quux"));
  ASSERT_EQ(-1, pu::ifind("quu", "quux"));
  ASSERT_EQ(-1, pu::ifind("", "quux"));

  // Find the empty string anywhere, so at 0.
  ASSERT_EQ(0, pu::ifind("foobar", ""));
  // .. except in an empty string
  ASSERT_EQ(-1, pu::ifind("", ""));
}
