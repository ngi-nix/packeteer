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
#include <packeteer/error.h>

#include <packeteer/detail/scheduler_impl.h>

#include <gtest/gtest.h>

#include <utility>
#include <chrono>

namespace pk = packeteer;
namespace sc = std::chrono;

namespace {

/**
 * Two callbacks, just so we have different function pointers to work with.
 **/
pk::error_t
foo(uint64_t, pk::error_t, pk::handle const &, void *)
{
  // no-op
  return pk::ERR_UNEXPECTED;
}

pk::error_t
bar(uint64_t, pk::error_t, pk::handle const &, void *)
{
  // no-op
  return pk::ERR_UNEXPECTED;
}

pk::error_t
baz(uint64_t, pk::error_t, pk::handle const &, void *)
{
  // no-op
  return pk::ERR_UNEXPECTED;
}

} // anonymous namespace


TEST(SchedulerContainers, io_callbacks)
{
  // We want to be able to find a range of eventmasks for a given
  // file descriptor. Since event masks are bitfields, and the index is
  // ordered, we should be able to find candidates quicker because we know
  // the event masks we're looking for will be >= the event that got
  // triggered.
  pk::detail::io_callbacks_t container;

  pk::detail::io_callback_entry * entry = new pk::detail::io_callback_entry(&foo,
      pk::handle::make_dummy(1), pk::PEV_IO_WRITE);
  container.add(entry);

  entry = new pk::detail::io_callback_entry(&bar, pk::handle::make_dummy(1),
      pk::PEV_IO_WRITE | pk::PEV_IO_READ);
  container.add(entry);

  entry = new pk::detail::io_callback_entry(&foo, pk::handle::make_dummy(1),
      pk::PEV_IO_READ);
  container.add(entry);

  entry = new pk::detail::io_callback_entry(&baz, pk::handle::make_dummy(1),
      pk::PEV_IO_READ);
  container.add(entry);

  entry = new pk::detail::io_callback_entry(&foo, pk::handle::make_dummy(2),
      pk::PEV_IO_READ);
  container.add(entry);

  // Two of the entries get merged, so we should have 3 entries for FD 1, and 
  // one entry for FD 2.

  // More precisely, there should be three read callbacks for FD 1
  auto range = container.copy_matching(pk::handle::make_dummy(1), pk::PEV_IO_READ);
  ASSERT_EQ(3, range.size());
  for (auto todelete : range) { delete todelete; }

  // There should be two write callbacks for FD 1
  range = container.copy_matching(pk::handle::make_dummy(1), pk::PEV_IO_WRITE);
  ASSERT_EQ(2, range.size());
  for (auto todelete : range) { delete todelete; }

  // There should be 1 read callback for FD 2
  range = container.copy_matching(pk::handle::make_dummy(2), pk::PEV_IO_READ);
  ASSERT_EQ(1, range.size());
  for (auto todelete : range) { delete todelete; }

  // And no write callback for FD 2
  range = container.copy_matching(pk::handle::make_dummy(2), pk::PEV_IO_WRITE);
  ASSERT_EQ(0, range.size());
  for (auto todelete : range) { delete todelete; }

  // Lastly, if we ask for callbacks for read or write, that should be three
  // again (for FD 1)
  range = container.copy_matching(pk::handle::make_dummy(1), pk::PEV_IO_READ | pk::PEV_IO_WRITE);
  ASSERT_EQ(3, range.size());
  for (auto todelete : range) { delete todelete; }
}


TEST(SchedulerContainers, scheduled_callbacks)
{
  // Ensure that constraints imposed on the container for scheduled callbacks
  // fulfil the requirements.

  // First, add three callbacks (we can ignore the callback function itself)
  // at two different m_timeout values. If the container works as intended,
  // the callback with the lowest timeout value will be found first on
  // iteration.
  pk::detail::scheduled_callbacks_t container;

  auto now = pk::clock::now();

  pk::detail::scheduled_callback_entry * entry = new pk::detail::scheduled_callback_entry(&foo,
      now + sc::microseconds(2));
  container.add(entry);

  entry = new pk::detail::scheduled_callback_entry(&bar, now + sc::microseconds(3));
  container.add(entry);

  entry = new pk::detail::scheduled_callback_entry(&foo, now + sc::microseconds(1));
  container.add(entry);

  entry = new pk::detail::scheduled_callback_entry(&baz, now + sc::microseconds(3));
  container.add(entry);

  auto timeout_index = container.get_timed_out(now);
  ASSERT_EQ(0, timeout_index.size());
  timeout_index = container.get_timed_out(now + sc::microseconds(2));
  ASSERT_EQ(2, timeout_index.size());
  timeout_index = container.get_timed_out(now + sc::microseconds(3));
  ASSERT_EQ(4, timeout_index.size());

  auto prev = now;
  for (auto value : timeout_index) {
    ASSERT_LE(prev, value->m_timeout);
    prev = value->m_timeout;
  }

  // Ensure that when we remove an entry, that's reflected in the timeout index
  entry = new pk::detail::scheduled_callback_entry(&foo, now + sc::microseconds(2));
  container.remove(entry);
  delete entry;

  timeout_index = container.get_timed_out(now);
  ASSERT_EQ(0, timeout_index.size());
  timeout_index = container.get_timed_out(now + sc::microseconds(3));
  ASSERT_EQ(2, timeout_index.size());

  prev = now;
  for (auto value : timeout_index) {
    ASSERT_LE(prev, value->m_timeout);
    prev = value->m_timeout;
  }
}


TEST(SchedulerContainers, user_callbacks)
{
  // The user callbacks container needs to fulfil two criteria. The simpler
  // one is that callbacks need to be found via a specific index. The trickier
  // one is that event masks need to be matched reasonably quickly, which
  // means finding entries with events >= a given event mask.
  enum user_events
  {
    EVENT_1 = 1 * pk::PEV_USER,
    EVENT_2 = 2 * pk::PEV_USER,
    EVENT_3 = 4 * pk::PEV_USER,
    EVENT_4 = 8 * pk::PEV_USER,
  };

  pk::detail::user_callbacks_t container;

  pk::detail::user_callback_entry * entry = new pk::detail::user_callback_entry(&foo, EVENT_1);
  container.add(entry);

  entry = new pk::detail::user_callback_entry(&bar, EVENT_3);
  container.add(entry);

  entry = new pk::detail::user_callback_entry(&baz, EVENT_1 | EVENT_3);
  container.add(entry);

  entry = new pk::detail::user_callback_entry(&bar, EVENT_1 | EVENT_2);
  container.add(entry);

  // Finding entries for the EVENT_1 mask should yield 3 entries, as adding
  // 'bar' the second time merges the entry with the first.
  auto range = container.copy_matching(EVENT_1);
  ASSERT_EQ(3, range.size());
  for (auto todelete : range) { delete todelete; }

  // Similarly, there should be one match for EVENT_2...
  range = container.copy_matching(EVENT_2);
  ASSERT_EQ(1, range.size());
  for (auto todelete : range) { delete todelete; }

  // ... two matches again for EVENT_3...
  range = container.copy_matching(EVENT_3);
  ASSERT_EQ(2, range.size());
  for (auto todelete : range) { delete todelete; }

  // ... and no matches again for EVENT_4.
  range = container.copy_matching(EVENT_4);
  ASSERT_EQ(0, range.size());
  for (auto todelete : range) { delete todelete; }

  // Now try to find entries with more complex masks.
  range = container.copy_matching(EVENT_1 | EVENT_2);
  ASSERT_EQ(3, range.size());
  for (auto todelete : range) { delete todelete; }

  range = container.copy_matching(EVENT_2 | EVENT_3);
  ASSERT_EQ(2, range.size());
  for (auto todelete : range) { delete todelete; }
}