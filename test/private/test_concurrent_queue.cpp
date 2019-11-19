/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 * Copyright (c) 2012-2014 Unwesen Ltd.
 * Copyright (c) 2015-2019 Jens Finkhaeuser.
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
#include "../../lib/concurrent_queue.h"

#include <gtest/gtest.h>

namespace pk = packeteer;


TEST(ConcurrentQueue, queue_functionality)
{
  // Test that the FIFO principle works for the queue as such, and that
  // extra functions do what they're supposed to do.
  pk::concurrent_queue<int> queue;
  ASSERT_EQ(true, queue.empty());
  ASSERT_EQ(0, queue.size());

  queue.push(42);
  ASSERT_EQ(false, queue.empty());
  ASSERT_EQ(1, queue.size());

  queue.push(666);
  ASSERT_EQ(false, queue.empty());
  ASSERT_EQ(2, queue.size());

  int value = 0;
  ASSERT_TRUE(queue.pop(value));
  ASSERT_EQ(42, value);
  ASSERT_EQ(false, queue.empty());
  ASSERT_EQ(1, queue.size());

  ASSERT_TRUE(queue.pop(value));
  ASSERT_EQ(666, value);
  ASSERT_EQ(true, queue.empty());
  ASSERT_EQ(0, queue.size());

  ASSERT_FALSE(queue.pop(value));
}
