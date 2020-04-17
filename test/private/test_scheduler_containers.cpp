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
#include <gtest/gtest.h>

#include "../../lib/scheduler/scheduler_impl.h"

#include <packeteer/error.h>

#include <utility>
#include <chrono>

namespace p7r = packeteer;
namespace sc = std::chrono;

namespace {

/**
 * Two callbacks, just so we have different function pointers to work with.
 **/
p7r::error_t
foo(p7r::events_t, p7r::error_t, p7r::connector const &, void *)
{
  // no-op
  return p7r::ERR_UNEXPECTED;
}

p7r::error_t
bar(p7r::events_t, p7r::error_t, p7r::connector const &, void *)
{
  // no-op
  return p7r::ERR_UNEXPECTED;
}

p7r::error_t
baz(p7r::events_t, p7r::error_t, p7r::connector const &, void *)
{
  // no-op
  return p7r::ERR_UNEXPECTED;
}

} // anonymous namespace


TEST(SchedulerContainers, io_callbacks)
{
  auto api = p7r::api::create();

  // Create & connect two anonymous connectors - before connecting, they will
  // be equal. Connecting one is enough.
  p7r::connector conn1{api, "anon://"};
  p7r::connector conn2{api, "anon://"};
  conn1.connect();
  // XXX conn2.connect();

  // We want to be able to find a range of eventmasks for a given
  // file descriptor. Since event masks are bitfields, and the index is
  // ordered, we should be able to find candidates quicker because we know
  // the event masks we're looking for will be >= the event that got
  // triggered.
  p7r::detail::io_callbacks_t container;

  p7r::detail::io_callback_entry * entry = new p7r::detail::io_callback_entry(&foo,
      conn1, p7r::PEV_IO_WRITE);
  container.add(entry);

  entry = new p7r::detail::io_callback_entry(&bar, conn1,
      p7r::PEV_IO_WRITE | p7r::PEV_IO_READ);
  container.add(entry);

  entry = new p7r::detail::io_callback_entry(&foo, conn1,
      p7r::PEV_IO_READ);
  container.add(entry);

  entry = new p7r::detail::io_callback_entry(&baz, conn1,
      p7r::PEV_IO_READ);
  container.add(entry);

  entry = new p7r::detail::io_callback_entry(&foo, conn2,
      p7r::PEV_IO_READ);
  container.add(entry);

  // Two of the entries get merged, so we should have 3 entries for FD 1, and 
  // one entry for FD 2.

  // More precisely, there should be three read callbacks for FD 1
  auto range = container.copy_matching(conn1, p7r::PEV_IO_READ);
  ASSERT_EQ(3, range.size());
  for (auto todelete : range) { delete todelete; }

  // There should be two write callbacks for FD 1
  range = container.copy_matching(conn1, p7r::PEV_IO_WRITE);
  ASSERT_EQ(2, range.size());
  for (auto todelete : range) { delete todelete; }

  // There should be 1 read callback for FD 2
  range = container.copy_matching(conn2, p7r::PEV_IO_READ);
  ASSERT_EQ(1, range.size());
  for (auto todelete : range) { delete todelete; }

  // And no write callback for FD 2
  range = container.copy_matching(conn2, p7r::PEV_IO_WRITE);
  ASSERT_EQ(0, range.size());
  for (auto todelete : range) { delete todelete; }

  // Lastly, if we ask for callbacks for read or write, that should be three
  // again (for FD 1)
  range = container.copy_matching(conn1, p7r::PEV_IO_READ | p7r::PEV_IO_WRITE);
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
  p7r::detail::scheduled_callbacks_t container;

  auto now = p7r::clock::now();

  p7r::detail::scheduled_callback_entry * entry = new p7r::detail::scheduled_callback_entry(&foo,
      now + sc::microseconds(2));
  container.add(entry);

  entry = new p7r::detail::scheduled_callback_entry(&bar, now + sc::microseconds(3));
  container.add(entry);

  entry = new p7r::detail::scheduled_callback_entry(&foo, now + sc::microseconds(1));
  container.add(entry);

  entry = new p7r::detail::scheduled_callback_entry(&baz, now + sc::microseconds(3));
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
  entry = new p7r::detail::scheduled_callback_entry(&foo, now + sc::microseconds(2));
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
    EVENT_1 = 1 * p7r::PEV_USER,
    EVENT_2 = 2 * p7r::PEV_USER,
    EVENT_3 = 4 * p7r::PEV_USER,
    EVENT_4 = 8 * p7r::PEV_USER,
  };

  p7r::detail::user_callbacks_t container;

  p7r::detail::user_callback_entry * entry = new p7r::detail::user_callback_entry(&foo, EVENT_1);
  container.add(entry);

  entry = new p7r::detail::user_callback_entry(&bar, EVENT_3);
  container.add(entry);

  entry = new p7r::detail::user_callback_entry(&baz, EVENT_1 | EVENT_3);
  container.add(entry);

  entry = new p7r::detail::user_callback_entry(&bar, EVENT_1 | EVENT_2);
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
