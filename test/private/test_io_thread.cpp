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
#include "../../lib/scheduler/io_thread.h"

#include <gtest/gtest.h>

#include "../test_name.h"
#include "../env.h"

#include <packeteer/util/tmp.h>
#include <packeteer/util/path.h>

#if defined(PACKETEER_HAVE_EPOLL_CREATE1)
#include "../../lib/scheduler/io/posix/epoll.h"
#endif

#if defined(PACKETEER_HAVE_SELECT)
#include "../../lib/scheduler/io/posix/select.h"
#endif

#if defined(PACKETEER_HAVE_POLL)
#include "../../lib/scheduler/io/posix/poll.h"
#endif

#if defined(PACKETEER_HAVE_KQUEUE)
#include "../../lib/scheduler/io/posix/kqueue.h"
#endif

#if defined(PACKETEER_HAVE_IOCP)
#include "../../lib/scheduler/io/win32/iocp.h"
#include "../../lib/scheduler/io/win32/select.h"
#endif


namespace p7r = packeteer;
namespace pu = packeteer::util;
namespace pd = packeteer::detail;
namespace sc = std::chrono;

using creator_func = std::function<pd::io * (std::shared_ptr<p7r::api> api)>;

struct test_data
{
  std::string   name;
  creator_func  creator;
  std::string   io_interrupt_name;
};

class IOThread
  : public testing::TestWithParam<test_data>
{
};


TEST_P(IOThread, simple_test)
{
  auto td = GetParam();

  // Create I/O subsystem
  auto io = std::unique_ptr<p7r::detail::io>(td.creator(test_env->api));

  // Start queue interrupt connector
  p7r::connector queue_interrupt{test_env->api, "anon://"};
  auto err = queue_interrupt.connect();
  ASSERT_EQ(p7r::ERR_SUCCESS, err);

  // Start IO interrupt connector
  p7r::connector io_interrupt{test_env->api, td.io_interrupt_name};
  err = io_interrupt.connect();
  ASSERT_EQ(p7r::ERR_SUCCESS, err);

  // Start thread
  p7r::detail::out_queue_t results;
  p7r::detail::io_thread thread{std::move(io),
    io_interrupt,
    results,
    queue_interrupt};
  err = thread.start();
  ASSERT_EQ(p7r::ERR_SUCCESS, err);

  // Wait for the thread to start.
  while (!thread.is_running()) {
    std::this_thread::sleep_for(sc::milliseconds(1));
  }
  std::this_thread::sleep_for(sc::milliseconds(50));

  // It's running... kill it.
  thread.stop();
  std::this_thread::sleep_for(sc::milliseconds(50));

  // After this, the results should contain exactly one read event for
  // our own dummy connector.
  p7r::detail::io_events events;
  size_t items = 0;
  while (results.pop(events)) {
    ++items;
    ASSERT_GE(events.size(), 1);
    for (auto event : events) {
      ASSERT_EQ(event.connector, io_interrupt);
    }
  }

  ASSERT_GE(items, 1);

  // The other thing that should happen is that reading from the
  // queue interrupt should yield some data.
  char buf[200] = { 0 };
  size_t bytes_read = 0;
  err = queue_interrupt.read(buf, sizeof(buf), bytes_read);
  ASSERT_EQ(p7r::ERR_SUCCESS, err);
  ASSERT_EQ(1, bytes_read);
}


namespace {
  test_data test_values[] = {
#if defined(PACKETEER_HAVE_EPOLL_CREATE1)
    test_data{
      "posix_epoll",
      [](std::shared_ptr<p7r::api> api) -> pd::io *
      {
        return new pd::io_epoll{api};
      },
      "anon://",
    },
#endif
#if defined(PACKETEER_HAVE_KQUEUE)
    {
      "posix_kqueue",
      [](std::shared_ptr<p7r::api> api) -> pd::io *
      {
        return new pd::io_kqueue{api};
      },
      "anon://",
    },
#endif
#if defined(PACKETEER_HAVE_POLL)
    {
      "posix_poll",
      [](std::shared_ptr<p7r::api> api) -> pd::io *
      {
        return new pd::io_poll{api};
      },
      "anon://",
    },
#endif
#if defined(PACKETEER_HAVE_SELECT)
    {
      "posix_select",
      [](std::shared_ptr<p7r::api> api) -> pd::io *
      {
        return new pd::io_select{api};
      },
      "anon://",
    },
#endif
#if defined(PACKETEER_HAVE_IOCP)
    {
      "win32_iocp",
      [](std::shared_ptr<p7r::api> api) -> pd::io *
      {
        return new pd::io_iocp{api};
      },
      "anon://",
    },
    {
      "win32_select",
      [](std::shared_ptr<p7r::api> api) -> pd::io *
      {
        return new pd::io_select{api};
      },
      "local://",
    },
#endif
  };
}


std::string io_name(testing::TestParamInfo<test_data> const & info)
{
  return info.param.name;
}


INSTANTIATE_TEST_SUITE_P(packeteer, IOThread,
    testing::ValuesIn(test_values),
    io_name);




/**
 * Miscellaneous tests
 */
TEST(IOThreadMisc, exception_in_io)
{
  struct test_io : public pd::io
  {
    test_io(std::shared_ptr<p7r::api> api)
      : io(api)
    {
    }

    void wait_for_events(pd::io_events &, p7r::duration const &)
    {
      throw std::runtime_error("Here's an error.");
    }
  };

  auto io = std::unique_ptr<p7r::detail::io>(new test_io{test_env->api});

  // Start queue interrupt connector
  p7r::connector queue_interrupt{test_env->api, "anon://"};
  auto err = queue_interrupt.connect();
  ASSERT_EQ(p7r::ERR_SUCCESS, err);

  // Start thread
  p7r::detail::out_queue_t results;
  p7r::detail::io_thread thread{std::move(io),
    queue_interrupt,
    results,
    queue_interrupt};

  err = thread.start();
  ASSERT_EQ(p7r::ERR_SUCCESS, err);

  // Wait for the thread to start.
  while (!thread.is_running()) {
    std::this_thread::sleep_for(sc::milliseconds(1));
  }
  std::this_thread::sleep_for(sc::milliseconds(50));

  // It should be dead at this point...
  ASSERT_FALSE(thread.is_running());
  auto exc = thread.error();
  ASSERT_NE(nullptr, exc);

  // Check caught exception
  try {
    std::rethrow_exception(exc);
  } catch (std::exception const & ex) {
    ASSERT_EQ(std::string{ex.what()}, "Here's an error.");
  }
}
