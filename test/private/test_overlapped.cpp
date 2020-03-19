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
#include "../../lib/connector/win32/overlapped.h"

#include <gtest/gtest.h>

#include "../test_name.h"

namespace p7r = packeteer;
namespace o = p7r::detail::overlapped;

namespace {
  // The HANDLE itself is not ever used - so we can set its pointer to any
  // arbitrary address.
  HANDLE * handle = reinterpret_cast<HANDLE *>(0xdeadbeef);
  HANDLE * other_handle = reinterpret_cast<HANDLE *>(0xdeadd00d);
} // anonymous namespace


TEST(OverlappedManager, enforce_parameters)
{
  // With an initial size of zero and grow_by set to double, we should get
  // errors on every schedule attempt.
  ASSERT_THROW((o::manager{0, -1}), p7r::exception);

  // The same with a no-grow policy.
  ASSERT_THROW((o::manager{0, 0}), p7r::exception);

  // But it should succeed if the pool may grow.
  ASSERT_NO_THROW((o::manager{0, 1}));
}



TEST(OverlappedManager, schedule_connect_with_restricted_pool)
{
  // Set the pool to be exactly and always one in size.
  o::manager m{1, 0};

  // Schedule a connect; this should result in a callback being invoked with
  // a SCHEDULE io_action.
  // In our first test case, the connect succeeds *immediately*.
  int called = 0;
  auto immediate_cb = [&called] (o::io_action action, o::io_context const & ctx) -> p7r::error_t
  {
    called += 1;

    EXPECT_EQ(o::SCHEDULE, action);
    EXPECT_EQ(o::CONNECT, ctx.type);
    EXPECT_EQ(handle, ctx.handle);

    // Other fields are of no real interest here.

    // Exit with immediate success!
    return p7r::ERR_SUCCESS;
  };

  auto err = m.schedule_overlapped(handle, o::CONNECT, immediate_cb);
  ASSERT_EQ(called, 1);
  ASSERT_EQ(p7r::ERR_SUCCESS, err);

  // Since the CONNECT succeeded immediately, we should be able to schedule
  // another immediate connect. It may not make sense, but it the overlapped
  // manager can't know that.
  called = 0;
  err = m.schedule_overlapped(handle, o::CONNECT, immediate_cb);
  ASSERT_EQ(called, 1);
  ASSERT_EQ(p7r::ERR_SUCCESS, err);

  // Next, let's schedule a callback that does not return immediately, and
  // signals this by returning ERR_ASYNC
  called = 0;
  auto deferred_cb = [&called] (o::io_action action, o::io_context const & ctx) -> p7r::error_t
  {
    called += 1;

    EXPECT_EQ(o::SCHEDULE, action);
    EXPECT_EQ(o::CONNECT, ctx.type);
    EXPECT_EQ(handle, ctx.handle);

    // Other fields are of no real interest here.

    // Exit signalling a deferred state.
    return p7r::ERR_ASYNC;
  };
  err = m.schedule_overlapped(handle, o::CONNECT, deferred_cb);
  ASSERT_EQ(called, 1);
  ASSERT_EQ(p7r::ERR_ASYNC, err);

  // Trying to schedule a new CONNECT request for the handle should now
  // lead to a CHECK_PROGRESS handle on the old one.
  called = false;
  auto check_cb = [&called] (o::io_action action, o::io_context const & ctx) -> p7r::error_t
  {
    called += 1;

    EXPECT_EQ(o::CHECK_PROGRESS, action);
    EXPECT_EQ(o::CONNECT, ctx.type);
    EXPECT_EQ(handle, ctx.handle);

    // Other fields are of no real interest here.

    // Don't finish yet.
    return p7r::ERR_ASYNC;
  };
  err = m.schedule_overlapped(handle, o::CONNECT, check_cb);
  ASSERT_EQ(called, 1);
  ASSERT_EQ(p7r::ERR_ASYNC, err);

  // Finally, since we're low on slots, scheduling anything for another handle
  // is not supported.
  called = 0;
  err = m.schedule_overlapped(other_handle, o::CONNECT, immediate_cb);
  ASSERT_EQ(called, 0);
  ASSERT_EQ(p7r::ERR_OUT_OF_MEMORY, err);
}



TEST(OverlappedManager, schedule_connect_with_growing_pool)
{
  // Let the pool grow with every request
  o::manager m{0, 1};

  // The callback we use never finishes.
  int called = 0;
  auto deferred_cb = [&called] (o::io_action action, o::io_context const & ctx) -> p7r::error_t
  {
    called += 1;

    EXPECT_EQ(o::SCHEDULE, action);
    EXPECT_EQ(o::CONNECT, ctx.type);

    // Other fields are of no real interest here.

    // Exit signalling a deferred state.
    return p7r::ERR_ASYNC;
  };

  // Scheduler connect for the first handle - expect success
  called = 0;
  auto err = m.schedule_overlapped(handle, o::CONNECT, deferred_cb);
  ASSERT_EQ(called, 1);
  ASSERT_EQ(p7r::ERR_ASYNC, err);

  // Schedule connect for the second handle - also expect success
  called = 0;
  err = m.schedule_overlapped(other_handle, o::CONNECT, deferred_cb);
  ASSERT_EQ(called, 1);
  ASSERT_EQ(p7r::ERR_ASYNC, err);
}
