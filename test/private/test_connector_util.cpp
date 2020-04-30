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

#include <packeteer/connector/types.h>

#include <gtest/gtest.h>

#include <cstring>

#include <sstream>
#include <string>
#include <set>
#include <bitset>

#include "../value_tests.h"
#include "../test_name.h"

#include "../lib/connector/util.h"

namespace p7r = packeteer;

TEST(ConnectorUtil, sanitize_options_good)
{
  auto defaults = p7r::CO_BLOCKING|p7r::CO_STREAM;
  auto possible = p7r::CO_STREAM|p7r::CO_BLOCKING|p7r::CO_NON_BLOCKING;

  // Just defaults
  {
    auto sanitized = p7r::detail::sanitize_options(p7r::CO_DEFAULT,
        defaults, possible);
    ASSERT_TRUE(p7r::CO_BLOCKING & sanitized);
    ASSERT_TRUE(p7r::CO_STREAM & sanitized);
    ASSERT_FALSE(p7r::CO_NON_BLOCKING & sanitized);
    ASSERT_FALSE(p7r::CO_DATAGRAM & sanitized);
  }

  // Captain obvious
  {
    auto sanitized = p7r::detail::sanitize_options(p7r::CO_BLOCKING,
        defaults, possible);
    ASSERT_TRUE(p7r::CO_BLOCKING & sanitized);
    ASSERT_TRUE(p7r::CO_STREAM & sanitized);
    ASSERT_FALSE(p7r::CO_NON_BLOCKING & sanitized);
    ASSERT_FALSE(p7r::CO_DATAGRAM & sanitized);
  }

  // Flip to non-blocking
  {
    auto sanitized = p7r::detail::sanitize_options(p7r::CO_NON_BLOCKING,
        defaults, possible);
    ASSERT_FALSE(p7r::CO_BLOCKING & sanitized);
    ASSERT_TRUE(p7r::CO_STREAM & sanitized);
    ASSERT_TRUE(p7r::CO_NON_BLOCKING & sanitized);
    ASSERT_FALSE(p7r::CO_DATAGRAM & sanitized);
  }
}



TEST(ConnectorUtil, sanitize_options_bad_defaults)
{
  auto defaults = p7r::CO_BLOCKING|p7r::CO_STREAM;
  auto possible = p7r::CO_DATAGRAM|p7r::CO_BLOCKING|p7r::CO_NON_BLOCKING;

  // The behaviour is leading over defaults
  {
    auto sanitized = p7r::detail::sanitize_options(p7r::CO_DEFAULT,
        defaults, possible);
    ASSERT_TRUE(p7r::CO_BLOCKING & sanitized);
    ASSERT_FALSE(p7r::CO_STREAM & sanitized);
    ASSERT_FALSE(p7r::CO_NON_BLOCKING & sanitized);
    ASSERT_TRUE(p7r::CO_DATAGRAM & sanitized);
  }

  {
    auto sanitized = p7r::detail::sanitize_options(p7r::CO_BLOCKING,
        defaults, possible);
    ASSERT_TRUE(p7r::CO_BLOCKING & sanitized);
    ASSERT_FALSE(p7r::CO_STREAM & sanitized);
    ASSERT_FALSE(p7r::CO_NON_BLOCKING & sanitized);
    ASSERT_TRUE(p7r::CO_DATAGRAM & sanitized);
  }
}



TEST(ConnectorUtil, sanitize_options_invalid_behaviour)
{
  auto defaults = p7r::CO_BLOCKING|p7r::CO_STREAM;

  // Missing behaviour
  ASSERT_THROW(p7r::detail::sanitize_options(p7r::CO_DEFAULT,
        defaults, p7r::CO_BLOCKING), std::logic_error);

  // Missing everything
  ASSERT_THROW(p7r::detail::sanitize_options(p7r::CO_DEFAULT,
        defaults, p7r::CO_DEFAULT), std::logic_error);

}


TEST(ConnectorUtil, sanitize_options_multi_behaviour)
{
  auto defaults = p7r::CO_BLOCKING|p7r::CO_STREAM;
  auto possible = p7r::CO_STREAM|p7r::CO_DATAGRAM|p7r::CO_BLOCKING;

  // Use default
  {
    auto sanitized = p7r::detail::sanitize_options(p7r::CO_DEFAULT,
        defaults, possible);
    ASSERT_TRUE(p7r::CO_STREAM & sanitized);
    ASSERT_FALSE(p7r::CO_DATAGRAM & sanitized);
  }

  // Use STREAM
  {
    auto sanitized = p7r::detail::sanitize_options(p7r::CO_STREAM,
        defaults, possible);
    ASSERT_TRUE(p7r::CO_STREAM & sanitized);
    ASSERT_FALSE(p7r::CO_DATAGRAM & sanitized);
  }

  // Use DATAGRAM
  {
    auto sanitized = p7r::detail::sanitize_options(p7r::CO_DATAGRAM,
        defaults, possible);
    ASSERT_FALSE(p7r::CO_STREAM & sanitized);
    ASSERT_TRUE(p7r::CO_DATAGRAM & sanitized);
  }

  // Without defaults, this throws.
  {
    ASSERT_THROW(p7r::detail::sanitize_options(p7r::CO_DEFAULT,
          p7r::CO_BLOCKING, possible), p7r::exception);
  }
}
