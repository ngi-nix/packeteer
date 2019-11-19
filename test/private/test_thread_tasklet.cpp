/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2014 Unwesen Ltd.
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

#include <gtest/gtest.h>

#include "../../lib/thread/tasklet.h"

#include <cstdlib>

#include "../compare_times.h"

#define THREAD_TEST_SHORT_DELAY std::chrono::milliseconds(1)
#define THREAD_TEST_LONG_DELAY  std::chrono::milliseconds(100)

namespace {

static bool done = false;

void sleeper(packeteer::thread::tasklet & t, void *)
{
  // Sleep until woken up.
  while (t.sleep()) {
    // tum-tee-tum.
  }
  done = true;
}

void sleep_halfsec(packeteer::thread::tasklet & t, void *)
{
  t.sleep(std::chrono::milliseconds(500));
  done = true;
}

static int count = 0;

void counter(packeteer::thread::tasklet & t, void *)
{
  while (t.sleep()) {
    ++count;
  }
}

struct bind_test
{
  bool finished;

  bind_test()
    : finished(false)
  {
  }

  void sleep_member(packeteer::thread::tasklet & t, void *)
  {
    // Sleep until woken up.
    while (t.sleep()) {
      // tum-tee-tum.
    }
    finished = true;
  }
};


} // anonymous namespace


// By testing the msec-based version of sleep(), both functions are
// tested nicely. It might be prudent to test them separately at some
// point though.

TEST(Tasklet, sleep_zero_msec)
{
  // Check whether sleep() correctly handles 0 msecs (no sleeping)
  done = false;
  packeteer::thread::tasklet task(sleeper);

  auto t1 = std::chrono::steady_clock::now();
  ASSERT_TRUE(task.start());
  std::this_thread::sleep_for(THREAD_TEST_LONG_DELAY);
  ASSERT_TRUE(task.stop());
  ASSERT_TRUE(task.wait());
  auto t2 = std::chrono::steady_clock::now();

  // The time difference must always be lower than the 1000 msecs
  // (1000000 nsecs) we specified as the sleep time.
  auto diff = t2 - t1;
  ASSERT_GT(diff, std::chrono::nanoseconds::zero());
  ASSERT_LT((t2 - t1).count(), 1'000'000'000);
}


TEST(Tasklet, sleep_some)
{
  // Same test, but with a half-second sleep. Now the elapsed time must
  // be larger than half a second.
  done = false;
  packeteer::thread::tasklet task(sleep_halfsec);

  auto t1 = std::chrono::steady_clock::now();
  ASSERT_TRUE(task.start());
  ASSERT_TRUE(task.wait());
  auto t2 = std::chrono::steady_clock::now();

  // The time difference must be very close to the sleep time of
  // 500msec. 
  compare_times(t1, t2, std::chrono::milliseconds(500));
}


TEST(Tasklet, sleep_count_wakeup)
{
  // Count how often the thread got woken. Since it sleeps indefinitely,
  // it should get woken exactly twice: once due to wakeup(), and once
  // due to stop(). The stop() one would not result in wake_count to be
  // incremented, though.
  count = 0;
  packeteer::thread::tasklet task(counter);

  ASSERT_TRUE(task.start());

  // Wait until the thread is running. Otherwise, wakeup() won't be
  // able to do anything (unless the thread runs quickly).
  std::this_thread::sleep_for(THREAD_TEST_LONG_DELAY);

  task.wakeup();

  // Wait until the wakup is handled. There's a possibility for a race
  // in that it's possible the thread has gone to sleep once more by
  // the time we're trying to wait on the condition, in which case the
  // thread will not notify us and stay in its sleep state.
  std::this_thread::sleep_for(THREAD_TEST_LONG_DELAY);

  ASSERT_EQ(1, count);

  ASSERT_TRUE(task.stop());
  ASSERT_TRUE(task.wait());
}


TEST(Tasklet, member_function)
{
  // Binding member functions is done much manually in packeteer.
  bind_test test;
  packeteer::thread::tasklet task(
      packeteer::thread::binder(test, &bind_test::sleep_member));

  auto t1 = std::chrono::steady_clock::now();
  ASSERT_TRUE(task.start());
  std::this_thread::sleep_for(THREAD_TEST_LONG_DELAY);
  ASSERT_TRUE(task.stop());
  ASSERT_TRUE(task.wait());
  auto t2 = std::chrono::steady_clock::now();

  // The time difference must always be lower than the 1000 msecs
  // (1000000 nsecs) we specified as the sleep time.
  auto diff = t2 - t1;
  ASSERT_GT(diff, std::chrono::nanoseconds::zero());
  ASSERT_LT((t2 - t1).count(), 1'000'000'000);
}


TEST(Tasklet, lambda)
{
  packeteer::thread::tasklet task([](packeteer::thread::tasklet &, void *) {
      std::this_thread::sleep_for(THREAD_TEST_SHORT_DELAY);
  });

  auto t1 = std::chrono::steady_clock::now();
  ASSERT_TRUE(task.start());
  std::this_thread::sleep_for(THREAD_TEST_LONG_DELAY);
  ASSERT_TRUE(task.stop());
  ASSERT_TRUE(task.wait());
  auto t2 = std::chrono::steady_clock::now();

  // The time difference must always be lower than the 1000 msecs
  // (1000000 nsecs) we specified as the sleep time.
  auto diff = t2 - t1;
  ASSERT_GT(diff, std::chrono::nanoseconds::zero());
  ASSERT_LT((t2 - t1).count(), 1'000'000'000);
}


TEST(Tasklet, scoped_behaviour)
{
  // Checks to determine whether tasklets that are destroyed before being
  // started, stopped or waited upon cause ugliness. These tests basically
  // only have to not segfault...

  // Unused task
  {
    packeteer::thread::tasklet task(sleeper);
  }

  // started task
  {
    packeteer::thread::tasklet task(sleeper);
    task.start();
  }

  // started & stopped task
  {
    packeteer::thread::tasklet task(sleeper);
    task.start();
    task.stop();
  }
}



TEST(Tasklet, shared_condition_variable)
{
  count = 0;

  std::condition_variable_any cond;
  std::recursive_mutex mutex;

  packeteer::thread::tasklet t1(&cond, &mutex, counter, nullptr, true);
  packeteer::thread::tasklet t2(&cond, &mutex, counter, nullptr, true);

  std::this_thread::sleep_for(THREAD_TEST_LONG_DELAY);

  t1.wakeup(); // also wakes up t2

  std::this_thread::sleep_for(THREAD_TEST_LONG_DELAY);

  // Both get woken
  ASSERT_LE(2, count);
  EXPECT_EQ(2, count) << "may fail under resource starvation.";

  // One gets stopped by this
  t1.stop();
  std::this_thread::sleep_for(THREAD_TEST_LONG_DELAY);
  ASSERT_LE(3, count);
  EXPECT_EQ(3, count) << "may fail under resource starvation.";

  // Now both should be stopped.
  t2.stop();
}
