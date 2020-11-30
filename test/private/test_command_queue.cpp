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

#include <string>

#include "../lib/command_queue.h"

namespace p7r = packeteer;
namespace pd = packeteer::detail;

TEST(DetailCommandQueue, enqueue_and_dequeue)
{
  using test_queue = pd::command_queue<int, std::string>;

  test_queue tq;

  tq.enqueue(42, "Hello");
  tq.enqueue(123, "world");

  int command = 0;
  std::string arg;

  bool ret = tq.dequeue(command, arg);
  ASSERT_TRUE(ret);
  ASSERT_EQ(42, command);
  ASSERT_EQ("Hello", arg);

  ret = tq.dequeue(command, arg);
  ASSERT_TRUE(ret);
  ASSERT_EQ(123, command);
  ASSERT_EQ("world", arg);

  ret = tq.dequeue(command, arg);
  ASSERT_FALSE(ret);
}



TEST(DetailCommandQueue, copy_counting)
{
  struct test
  {
    test() {}
    test(test const & t) { copies = t.copies + 1; };
    int copies = 0;
  };

  using test_queue = pd::command_queue<int, test>;

  test_queue tq;
  tq.enqueue(42, test{});

  int command;
  test result;
  bool ret = tq.dequeue(command, result);
  ASSERT_TRUE(ret);

  // Copy for enqueueing, and copy for dequeueing.
  ASSERT_EQ(2, result.copies);
}



TEST(DetailCommandQueueWithSignal, signalling)
{
  using test_queue = pd::command_queue_with_signal<int, std::string>;

  p7r::connector conn{p7r::api::create(), "anon://"};
  auto err = conn.connect();
  ASSERT_EQ(p7r::ERR_SUCCESS, err);

  test_queue tq{conn};

  tq.enqueue(42, "foo"); // does not really matter
  tq.commit();

  auto ret = tq.clear();
  ASSERT_TRUE(ret);

  // The interrupt can be cleared and queried independent of whether
  // the queue has entries.
}
