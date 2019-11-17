/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2017-2019 Jens Finkhaeuser.
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

#include <packeteer/error.h>

#include <errno.h>

#include <string>

#include <gtest/gtest.h>


TEST(Error, basics)
{
  try {
    throw packeteer::exception(packeteer::ERR_SUCCESS);
  } catch (packeteer::exception const & ex) {
    ASSERT_EQ(packeteer::ERR_SUCCESS, ex.code());
    ASSERT_EQ(std::string("No error"), std::string(ex.what()));
    ASSERT_EQ(std::string("ERR_SUCCESS"), std::string(ex.name()));
  }
}


TEST(Error, details_without_errno)
{
  // Without errno
  try {
    throw packeteer::exception(packeteer::ERR_SUCCESS, "foo");
  } catch (packeteer::exception const & ex) {
    ASSERT_EQ(std::string("foo"), ex.details());
  }
}


TEST(Error, details_with_errno)
{
  // With an errno value
  try {
    throw packeteer::exception(packeteer::ERR_SUCCESS, EAGAIN, "foo");
  } catch (packeteer::exception const & ex) {

    std::string prefix = "foo // ";
    ASSERT_GT(ex.details().size(), prefix.size());
    ASSERT_EQ(0, ex.details().compare(0, prefix.size(), prefix));
  }
}
