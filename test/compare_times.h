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

#ifndef PACKETEER_TEST_COMPARE_TIMES_H
#define PACKETEER_TEST_COMPARE_TIMES_H

#include <cppunit/extensions/HelperMacros.h>

#include <chrono>

template <typename firstT, typename secondT, typename expectedT>
inline void
compare_times(firstT const & first, secondT const & second, expectedT const & expected)
{
  namespace sc = std::chrono;

  auto diff = sc::duration_cast<sc::nanoseconds>(second.time_since_epoch()) -
    sc::duration_cast<sc::nanoseconds>(first.time_since_epoch());

  CPPUNIT_ASSERT_MESSAGE("Cannot spend negative time!", diff.count() > 0);

  // The diff should be no more than 25% off of the expected value, but also it's possible it will be >20ms due to scheduler granularity.
  auto max = sc::duration_cast<sc::nanoseconds>(expected * 1.25);
  auto twenty = sc::milliseconds(20);
  if (max < twenty) {
    max = twenty;
  }

  CPPUNIT_ASSERT_MESSAGE("Should not fail except under high CPU workload or on emulators!", diff.count() <= max.count());
}

#endif // guard
