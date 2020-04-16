/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2017-2020 Jens Finkhaeuser.
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
#include <packeteer/handle.h>

#include <sstream>

#include <gtest/gtest.h>

namespace pk = packeteer;

TEST(Handle, test_basic_functionality)
{
  // Default-constructed handles are invalid
  pk::handle h1;
  ASSERT_FALSE(h1.valid());

  // Two default-constructed handles should have the same hash value.
  pk::handle h2;
  ASSERT_EQ(h1.hash(), h2.hash());

  // In fact, they should be equal
  ASSERT_EQ(h1, h2);
  ASSERT_FALSE(h1 < h2);
  ASSERT_FALSE(h2 < h1);

  // Copying should not change this at all
  pk::handle copy = h2;
  ASSERT_EQ(h1, copy);
  ASSERT_FALSE(h1 < copy);
  ASSERT_FALSE(copy < h1);

  // Outputting a handle should output its hash
  std::stringstream s1;
  s1 << h1;

  std::stringstream s2;
  s2 << h1.hash();
  ASSERT_EQ(s1.str(), s2.str());
}


TEST(Handle, test_dummy)
{
  auto h1 = pk::handle::make_dummy(1);
  auto h2 = pk::handle::make_dummy(2);

  ASSERT_TRUE(h1.valid());
  ASSERT_TRUE(h2.valid());
  ASSERT_NE(h1, h2);

  auto h3 = h2;
  ASSERT_NE(h1, h3);
  ASSERT_EQ(h2, h3);
}
