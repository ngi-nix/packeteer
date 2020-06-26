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
#include "../env.h"

#include <packeteer/scheduler.h>
#include <packeteer/connector.h>

#include "../lib/macros.h"

#include <utility>
#include <atomic>

#include <thread>
#include <chrono>

#define TEST_SLEEP_TIME std::chrono::milliseconds(50)

namespace p7r = packeteer;
namespace sc = std::chrono;

namespace {

/**
 * Test callback. We're making it a member function so we can easily hand it
 * some parameters.
 **/
struct test_callback
{
  std::atomic<int>            m_called;
  std::atomic<p7r::events_t>  m_mask;

  test_callback()
    : m_called(0)
    , m_mask(0)
  {
  }


  p7r::error_t
  func(p7r::time_point const & now [[maybe_unused]],
      p7r::events_t mask,
      p7r::connector * conn [[maybe_unused]])
  {
    ++m_called;
    m_mask = mask;
    std::string connid = "nullptr conn";
    if (conn) {
      connid = std::to_string(*conn);
    }
    DLOG("callback called: " << connid << " - " << mask << " [called: " << m_called << "]");

    return p7r::error_t(0);
  }
};


struct counting_callback
{
  std::atomic<int> m_read_called = 0;
  std::atomic<int> m_write_called = 0;

  p7r::error_t
  func(p7r::time_point const & now [[maybe_unused]], p7r::events_t mask,
      p7r::connector * conn [[maybe_unused]])
  {
    if (mask & p7r::PEV_IO_READ) {
      ++m_read_called;
    }
    if (mask & p7r::PEV_IO_WRITE) {
      ++m_write_called;
    }
    return p7r::error_t(0);
  }

};



struct thread_id_callback
{
  std::thread::id  m_tid;

  thread_id_callback()
    : m_tid()
  {
  }


  p7r::error_t
  func(p7r::time_point const & now [[maybe_unused]], p7r::events_t,
      p7r::connector *)
  {
    m_tid = std::this_thread::get_id();

    DLOG("callback started");
    std::this_thread::sleep_for(TEST_SLEEP_TIME);
    DLOG("callback ended");

    return p7r::error_t(0);
  }
};


struct reading_callback : public test_callback
{
  p7r::connector & m_conn;
  size_t m_read;
  int m_called_before_read;

  reading_callback(p7r::connector & conn)
    : test_callback()
    , m_conn(conn)
    , m_read(0)
    , m_called_before_read(-1)
  {
  }

  p7r::error_t
  func(p7r::time_point const & now, p7r::events_t mask,
      p7r::connector * h)
  {
    p7r::error_t err = test_callback::func(now, mask, h);
    if (err != 0) {
      return err;
    }

    if (m_called_before_read < 0) {
      m_called_before_read = m_called;
      char buf[200];
      err = m_conn.read(buf, sizeof(buf), m_read);
    }
    return err;
  }
};




/**
 * Help verify callback expectations.
 **/
#define ASSERT_CALLBACK(cb, expected_called, expected_mask) \
  {                                     \
    int called = cb.m_called;           \
    ASSERT_EQ(expected_called, called); \
    p7r::events_t mask = cb.m_mask;          \
    ASSERT_EQ(expected_mask, mask);     \
    cb.m_mask = 0; /* reset mask */     \
  }

#define ASSERT_CALLBACK_GREATER(cb, expected_called, expected_mask) \
  {                                     \
    int called = cb.m_called;           \
    ASSERT_GT(called, expected_called); \
    p7r::events_t mask = cb.m_mask;          \
    ASSERT_EQ(expected_mask, mask);     \
    cb.m_mask = 0; /* reset mask */     \
  }


std::string scheduler_name(testing::TestParamInfo<packeteer::scheduler::scheduler_type> const & info)
{
  switch (info.param) {
    case packeteer::scheduler::TYPE_AUTOMATIC:
      return "automatic";

    case packeteer::scheduler::TYPE_EPOLL:
      return "epoll";

    case packeteer::scheduler::TYPE_KQUEUE:
      return "kqueue";

    case packeteer::scheduler::TYPE_POLL:
      return "poll";

    case packeteer::scheduler::TYPE_SELECT:
      return "select";

    case packeteer::scheduler::TYPE_WIN32:
      return "win32";

    default:
      ADD_FAILURE_AT(__FILE__, __LINE__) << "Test not defined for scheduler type " << info.param;
  }

  return {}; // Silence warning
}

} // anonymous namespace


class Scheduler
  : public testing::TestWithParam<packeteer::scheduler::scheduler_type>
{
};


TEST_P(Scheduler, delayed_callback)
{
  auto td = GetParam();

  // We only need one thread for this.
  p7r::scheduler sched(test_env->api, 0, static_cast<p7r::scheduler::scheduler_type>(td));

  test_callback source;
  p7r::callback cb{&source, &test_callback::func};

  sched.schedule_once(sc::milliseconds(1), cb);

  sched.process_events(sc::milliseconds(20));

  int called = source.m_called;
  ASSERT_EQ(1, called);

  p7r::events_t mask = source.m_mask;
  ASSERT_EQ(p7r::PEV_TIMEOUT, mask);
}



TEST_P(Scheduler, soft_timeout)
{
  auto td = GetParam();

  // We only need one thread for this.
  p7r::scheduler sched(test_env->api, 0, static_cast<p7r::scheduler::scheduler_type>(td));

  test_callback source;
  p7r::callback cb{&source, &test_callback::func};

  sched.schedule_once(TEST_SLEEP_TIME, cb);

  auto before = p7r::clock::now();
  sched.process_events(sc::milliseconds(1), true);
  auto after = p7r::clock::now();

  int called = source.m_called;
  ASSERT_EQ(1, called);

  p7r::events_t mask = source.m_mask;
  ASSERT_EQ(p7r::PEV_TIMEOUT, mask);

  // Even though we waited for 1 millisecond only, due to the soft timeout
  // and the next scheduled callback at 50 msec, we have to have
  // 50+ msec elapsed
  auto elapsed = sc::round<sc::milliseconds>(after - before).count();
  ASSERT_GE(elapsed, 50);
}



TEST_P(Scheduler, timed_callback)
{
  auto td = GetParam();

  p7r::scheduler sched(test_env->api, 0, static_cast<p7r::scheduler::scheduler_type>(td));

  test_callback source;
  p7r::callback cb{&source, &test_callback::func};

  sched.schedule_at(p7r::clock::now() + TEST_SLEEP_TIME, cb);

  sched.process_events(sc::milliseconds(100));

  int called = source.m_called;
  ASSERT_EQ(1, called);

  p7r::events_t mask = source.m_mask;
  ASSERT_EQ(p7r::PEV_TIMEOUT, mask);
}


TEST_P(Scheduler, repeat_callback)
{
  auto td = GetParam();

  // We only need one thread for this.
  p7r::scheduler sched(test_env->api, 0, static_cast<p7r::scheduler::scheduler_type>(td));

  test_callback source;
  p7r::callback cb{&source, &test_callback::func};

  sched.schedule(p7r::clock::now(), sc::milliseconds(20),
      3, cb);

  // If we process multiple times, each time the expiring callback should
  // kick us out of the loop - but no more than three times. The last wait
  // needs to time out.
  sched.process_events(TEST_SLEEP_TIME);
  sched.process_events(TEST_SLEEP_TIME);
  sched.process_events(TEST_SLEEP_TIME);
  sched.process_events(TEST_SLEEP_TIME);

  int called = source.m_called;
  ASSERT_EQ(3, called);

  p7r::events_t mask = source.m_mask;
  ASSERT_EQ(p7r::PEV_TIMEOUT, mask);
}


TEST_P(Scheduler, infinite_callback)
{
  auto td = GetParam();

  // Infinite callbacks are easy enough to test for in that the callback
  // must have been invoked more than once just as above. However, once
  // explicitly unscheduled, the callback cannot be invoked any longer.

  p7r::scheduler sched(test_env->api, 0, static_cast<p7r::scheduler::scheduler_type>(td));

  test_callback source;
  p7r::callback cb{&source, &test_callback::func};

  auto now = p7r::clock::now();

  sched.schedule(now, TEST_SLEEP_TIME, cb);

  // Since the first invocation happens immediately, we want to sleep <
  // 3 * 50 msec
  sched.process_events(TEST_SLEEP_TIME);
  sched.process_events(TEST_SLEEP_TIME);
  sched.process_events(TEST_SLEEP_TIME);

  int called = source.m_called;
  ASSERT_EQ(3, called);

  p7r::events_t mask = source.m_mask;
  ASSERT_EQ(p7r::PEV_TIMEOUT, mask);

  sched.unschedule(cb);

  sched.process_events(TEST_SLEEP_TIME);

  // The amount of invocations may not have changed after the unschedule()
  // call above, even though we waited longer.
  called = source.m_called;
  ASSERT_EQ(3, called);

  mask = source.m_mask;
  ASSERT_EQ(p7r::PEV_TIMEOUT, mask);
}


TEST_P(Scheduler, delayed_repeat_callback)
{
  auto td = GetParam();

  // Repeat every 20 msec, but delay for 50msec
  sc::milliseconds interval = sc::milliseconds(20);
  auto delay = p7r::clock::now() + TEST_SLEEP_TIME;

  p7r::scheduler sched(test_env->api, 0, static_cast<p7r::scheduler::scheduler_type>(td));

  test_callback source;
  p7r::callback cb{&source, &test_callback::func};

  sched.schedule(delay, interval, -1, cb);

  // If we process for < 50 msec, the callback should not be invoked.
  sched.process_events(sc::milliseconds(20));
  EXPECT_EQ(0, source.m_called) << "IOCP sometimes sleeps longer than specified.";

  // Now if we wait another 30 (left over delay) plus 20
  // msec, we should have a callbacks.
  sched.process_events(sc::milliseconds(30 + 20));
  ASSERT_EQ(1, source.m_called);

  p7r::events_t mask = source.m_mask;
  ASSERT_EQ(p7r::PEV_TIMEOUT, mask);

  sched.unschedule(cb);
}


TEST_P(Scheduler, parallel_callback_with_threads)
{
  auto td = GetParam();

  // Test that callbacks are executed in parallel by scheduling two at the
  // same time, and using two worker threads. Each callback sleeps for a while
  // and remembers its thread id; the two callbacks need to have different
  // thread ids afterwards for this to succeed.

  // We need >1 thread to enable parallell processing.
  p7r::scheduler sched(test_env->api, 2, static_cast<p7r::scheduler::scheduler_type>(td));

  thread_id_callback source1;
  p7r::callback cb1{&source1, &thread_id_callback::func};
  thread_id_callback source2;
  p7r::callback cb2{&source2, &thread_id_callback::func};

  sched.schedule_once(TEST_SLEEP_TIME, cb1);
  sched.schedule_once(TEST_SLEEP_TIME, cb2);

  std::this_thread::sleep_for(sc::milliseconds(150));

  std::thread::id id1 = source1.m_tid;
  std::thread::id id2 = source2.m_tid;
  ASSERT_NE(id1, id2);
}


TEST_P(Scheduler, user_callback)
{
  auto td = GetParam();

  // We register the same callback for two user-defined events; firing either
  // must cause the callback to be invoked.
  // Unregistering the callback from one of the events must cause the callback
  // to only be invoked for the other.
  enum user_events
  {
    EVENT_1 = 1 * p7r::PEV_USER,
    EVENT_2 = 2 * p7r::PEV_USER,
    EVENT_3 = 4 * p7r::PEV_USER,
  };

  p7r::scheduler sched(test_env->api, 0, static_cast<p7r::scheduler::scheduler_type>(td));

  test_callback source1;
  p7r::callback cb1{&source1, &test_callback::func};
  sched.register_event(EVENT_1 | EVENT_2 | EVENT_3, cb1);

  test_callback source2;
  p7r::callback cb2{&source2, &test_callback::func};
  sched.register_event(EVENT_2 | EVENT_3, cb2);

  ASSERT_NE(cb1, cb2);
  ASSERT_NE(cb1.hash(), cb2.hash());

  // EVENT_1
  sched.fire_events(EVENT_1);
  sched.process_events(sc::milliseconds(0));

  ASSERT_CALLBACK(source1, 1, EVENT_1);
  ASSERT_CALLBACK(source2, 0, 0);

  // EVENT_2
  sched.fire_events(EVENT_2);
  sched.process_events(sc::milliseconds(0));

  ASSERT_CALLBACK(source1, 2, EVENT_2);
  ASSERT_CALLBACK(source2, 1, EVENT_2);

  // EVENT_3
  sched.fire_events(EVENT_3);
  sched.process_events(sc::milliseconds(0));

  ASSERT_CALLBACK(source1, 3, EVENT_3);
  ASSERT_CALLBACK(source2, 2, EVENT_3);

  // EVENT_1 | EVENT_2
  sched.fire_events(EVENT_1 | EVENT_2);
  sched.process_events(sc::milliseconds(0));

  ASSERT_CALLBACK(source1, 4, EVENT_1 | EVENT_2);
  ASSERT_CALLBACK(source2, 3, EVENT_2);

  // EVENT_2 | EVENT_3
  sched.fire_events(EVENT_2 | EVENT_3);
  sched.process_events(sc::milliseconds(0));

  ASSERT_CALLBACK(source1, 5, EVENT_2 | EVENT_3);
  ASSERT_CALLBACK(source2, 4, EVENT_2 | EVENT_3);

  // EVENT_1 | EVENT_3
  sched.fire_events(EVENT_1 | EVENT_3);
  sched.process_events(sc::milliseconds(0));

  ASSERT_CALLBACK(source1, 6, EVENT_1 | EVENT_3);
  ASSERT_CALLBACK(source2, 5, EVENT_3);

  // Unregister one from EVENT_2
  sched.unregister_event(EVENT_2, cb1);

  // EVENT_1
  sched.fire_events(EVENT_1);
  sched.process_events(sc::milliseconds(0));

  ASSERT_CALLBACK(source1, 7, EVENT_1);
  ASSERT_CALLBACK(source2, 5, 0); // mask reset; not called

  // EVENT_3
  sched.fire_events(EVENT_2);
  sched.process_events(sc::milliseconds(0));

  ASSERT_CALLBACK(source1, 7, 0); // mask reset; not called
  ASSERT_CALLBACK(source2, 6, EVENT_2);

  // EVENT_3
  sched.fire_events(EVENT_3);
  sched.process_events(sc::milliseconds(0));

  ASSERT_CALLBACK(source1, 8, EVENT_3);
  ASSERT_CALLBACK(source2, 7, EVENT_3);

  // EVENT_1 | EVENT_2
  sched.fire_events(EVENT_1 | EVENT_2);
  sched.process_events(sc::milliseconds(0));

  ASSERT_CALLBACK(source1, 9, EVENT_1);
  ASSERT_CALLBACK(source2, 8, EVENT_2);

  // EVENT_2 | EVENT_3
  sched.fire_events(EVENT_2 | EVENT_3);
  sched.process_events(sc::milliseconds(0));

  ASSERT_CALLBACK(source1, 10, EVENT_3);
  ASSERT_CALLBACK(source2, 9, EVENT_2 | EVENT_3);

  // EVENT_1 | EVENT_3
  sched.fire_events(EVENT_1 | EVENT_3);
  sched.process_events(sc::milliseconds(0));

  ASSERT_CALLBACK(source1, 11, EVENT_1 | EVENT_3);
  ASSERT_CALLBACK(source2, 10, EVENT_3);


  // Also ensure that fire_event() does not work with system events.
  ASSERT_EQ(p7r::ERR_INVALID_VALUE,
      sched.fire_events(p7r::PEV_IO_READ));
}


TEST_P(Scheduler, io_callback)
{
  auto td = GetParam();

  // The simplest way to test I/O callbacks is with a pipe.
  p7r::connector pipe{test_env->api, "anon://"};
  pipe.connect();

  p7r::scheduler sched(test_env->api, 0, static_cast<p7r::scheduler::scheduler_type>(td));

  test_callback source1;
  p7r::callback cb1{&source1, &test_callback::func};
  sched.register_connector(p7r::PEV_IO_READ, pipe, cb1);

  test_callback source2;
  p7r::callback cb2{&source2, &test_callback::func};
  sched.register_connector(p7r::PEV_IO_WRITE, pipe, cb2);
  sched.process_events(TEST_SLEEP_TIME);

  sched.unregister_connector(p7r::PEV_IO_WRITE, pipe, cb2);
  sched.process_events(TEST_SLEEP_TIME);

  // The second callback must have been invoked multiple times, because the
  // pipe is always (at this level of I/O load) writeable.
  ASSERT_CALLBACK_GREATER(source2, 0, p7r::PEV_IO_WRITE);

  // On the other hand, without writing to the pipe, we should not have any
  // callbacks for reading.
  ASSERT_CALLBACK(source1, 0, 0);
  sched.unregister_connector(p7r::PEV_IO_WRITE, pipe, cb1);

  reading_callback reading(pipe);
  p7r::callback rd{&reading, &reading_callback::func};
  sched.register_connector(p7r::PEV_IO_READ, pipe, rd);
  sched.process_events(TEST_SLEEP_TIME);

  // So let's write something to the pipe. This will trigger the read callback
  // until we're reading from the pipe again.
  char buf[] = { '\0' };
  size_t amount = 0;
  pipe.write(buf, sizeof(buf), amount);
  ASSERT_EQ(sizeof(buf), amount);

  sched.process_events(TEST_SLEEP_TIME);

  // After writing, there must be a callback
  ASSERT_CALLBACK_GREATER(reading, 0, p7r::PEV_IO_READ);

  // We may have been called multiple times, but we should only have been
  // called once before reading from the pipe.
  ASSERT_EQ(1, reading.m_called_before_read) << "Should never be called more than once before reading.";

  // After reading, we might be called more often, but it shouldn't be that
  // much - this is difficult to bound, because it's the thread scheduling
  // and I/O scheduling properties of the kernel that determine this.
  ASSERT_TRUE(reading.m_called >= 1 && reading.m_called < 100) << "Should not fail.";
}



TEST_P(Scheduler, io_callback_remove_all_callbacks)
{
  auto td = GetParam();

  // The simplest way to test I/O callbacks is with a pipe.
  p7r::connector pipe{test_env->api, "anon://"};
  pipe.connect();

  p7r::scheduler sched(test_env->api, 0, static_cast<p7r::scheduler::scheduler_type>(td));

  counting_callback source1;
  p7r::callback cb1{&source1, &counting_callback::func};
  sched.register_connector(p7r::PEV_IO_WRITE, pipe, cb1);

  counting_callback source2;
  p7r::callback cb2{&source2, &counting_callback::func};

  // Register callbacks and process
  sched.register_connector(p7r::PEV_IO_WRITE, pipe, cb2);
  sched.process_events(TEST_SLEEP_TIME);

  // Both callbacks must have been invoked.
  ASSERT_GE(source1.m_write_called, 0);
  ASSERT_GE(source2.m_write_called, 0);

  // Now if we unregister the entire connector without specifying a
  // callback, we should not get more callbacks.
  int s1 = source1.m_write_called;
  int s2 = source2.m_write_called;

  sched.unregister_connector(p7r::PEV_IO_WRITE, pipe);
  sched.process_events(TEST_SLEEP_TIME);

  ASSERT_EQ(s1, source1.m_write_called);
  ASSERT_EQ(s2, source2.m_write_called);
}



TEST_P(Scheduler, io_callback_remove_completely)
{
  auto td = GetParam();

  // The simplest way to test I/O callbacks is with a pipe.
  p7r::connector pipe{test_env->api, "anon://"};
  pipe.connect();

  p7r::scheduler sched(test_env->api, 0, static_cast<p7r::scheduler::scheduler_type>(td));

  counting_callback source1;
  p7r::callback cb1{&source1, &counting_callback::func};
  sched.register_connector(p7r::PEV_IO_WRITE, pipe, cb1);

  counting_callback source2;
  p7r::callback cb2{&source2, &counting_callback::func};

  // Register callbacks and process
  sched.register_connector(p7r::PEV_IO_WRITE, pipe, cb2);
  sched.process_events(TEST_SLEEP_TIME);

  // Both callbacks must have been invoked.
  ASSERT_GE(source1.m_write_called, 0);
  ASSERT_GE(source2.m_write_called, 0);

  // Now if we unregister the entire connector, we should not get any more
  // callbacks.
  int s1 = source1.m_write_called;
  int s2 = source2.m_write_called;

  sched.unregister_connector(pipe);
  sched.process_events(TEST_SLEEP_TIME);

  ASSERT_EQ(s1, source1.m_write_called);
  ASSERT_EQ(s2, source2.m_write_called);
}



TEST_P(Scheduler, io_callback_registration_simultaneous)
{
  auto td = GetParam();

  // First case registers read/write callbacks simultaneously.
  p7r::connector pipe{test_env->api, "anon://"};
  pipe.connect();

  p7r::scheduler sched(test_env->api, 0, static_cast<p7r::scheduler::scheduler_type>(td));

  counting_callback source;
  p7r::callback cb{&source, &counting_callback::func};
  sched.register_connector(p7r::PEV_IO_READ|p7r::PEV_IO_WRITE, pipe, cb);
  sched.process_events(TEST_SLEEP_TIME);

  // No read callbacks without writing.
  ASSERT_EQ(source.m_read_called, 0);

  // Writing should trigger an invocation.
  char buf[] = { '\0' };
  size_t amount = 0;
  pipe.write(buf, sizeof(buf), amount);
  ASSERT_EQ(sizeof(buf), amount);

  sched.process_events(TEST_SLEEP_TIME);

  // After writing, there must be a callback
  ASSERT_GT(source.m_read_called, 0);
}



TEST_P(Scheduler, io_callback_registration_sequence)
{
  auto td = GetParam();

  // Second case registers them one after another, which could lead to overwrites.
  p7r::connector pipe{test_env->api, "anon://"};
  pipe.connect();

  p7r::scheduler sched(test_env->api, 0, static_cast<p7r::scheduler::scheduler_type>(td));

  counting_callback source;
  p7r::callback cb{&source, &counting_callback::func};
  sched.register_connector(p7r::PEV_IO_READ|p7r::PEV_IO_WRITE, pipe, cb);

  sched.process_events(TEST_SLEEP_TIME);

  // No read callbacks without writing.
  ASSERT_EQ(source.m_read_called, 0);

  // Writing should trigger an invocation.
  char buf[] = { '\0' };
  size_t amount = 0;
  pipe.write(buf, sizeof(buf), amount);
  ASSERT_EQ(sizeof(buf), amount);

  sched.process_events(TEST_SLEEP_TIME);

  // After writing, there must be a callback
  ASSERT_GT(source.m_read_called, 0);
}


TEST_P(Scheduler, worker_count)
{
  auto td = GetParam();

  p7r::scheduler sched(test_env->api, -1, static_cast<p7r::scheduler::scheduler_type>(td));

  // With -1, the scheduler should determine the number of workers themselves.
  ASSERT_GT(sched.num_workers(), 0);
}


namespace {
  auto test_values = []
  {
    return testing::Values(
        packeteer::scheduler::TYPE_AUTOMATIC
#if defined(PACKETEER_HAVE_EPOLL_CREATE1)
      , packeteer::scheduler::TYPE_EPOLL
#endif
#if defined(PACKETEER_HAVE_KQUEUE)
      , packeteer::scheduler::TYPE_KQUEUE
#endif
#if defined(PACKETEER_HAVE_POLL)
      , packeteer::scheduler::TYPE_POLL
#endif
#if defined(PACKETEER_HAVE_SELECT)
      , packeteer::scheduler::TYPE_SELECT
#endif
#if defined(PACKETEER_HAVE_IOCP)
      , packeteer::scheduler::TYPE_WIN32
#endif
    );
  };
}

INSTANTIATE_TEST_SUITE_P(packeteer, Scheduler,
    test_values(),
    scheduler_name);
