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

#include <packeteer/util/tmp.h>

#include <gtest/gtest.h>

namespace util = packeteer::util;

TEST(TmpDir, has_value)
{
  auto tmp = util::temp_name();
  // std::cout << "TMP: "<< tmp << std::endl;

  // Ask for a few characters at least
  ASSERT_GT(tmp.size(), 3);
}



TEST(TmpDir, contains_prefix)
{
  auto tmp = util::temp_name("p7r");
  // std::cout << "TMP: "<< tmp << std::endl;
  EXPECT_GT(tmp.size(), 3);

  auto pos = tmp.find("p7r");
  ASSERT_NE(pos, std::string::npos);
}
