/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 * Copyright (c) 2012-2014 Unwesen Ltd.
 * Copyright (c) 2015-2020 Jens Finkhaeuser.
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
#include <iostream>

#include <gtest/gtest.h>

#include "env.h"

#include <packeteer.h>

TestEnvironment * test_env = nullptr;

int main(int argc, char **argv)
{
  std::cout << packeteer::copyright_string() << std::endl;

  try {
    test_env = new TestEnvironment();
  } catch (packeteer::exception const & ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }

  // Ownership passes to gtest here.
  ::testing::AddGlobalTestEnvironment(test_env);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
