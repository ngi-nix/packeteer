/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 * Copyright (c) 2012-2014 Unwesen Ltd.
 * Copyright (c) 2015-2017 Jens Finkhaeuser.
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
#include <packeteer/scheduler.h>

#include <cppunit/extensions/HelperMacros.h>

#include <utility>
#include <atomic>

#include <twine/thread.h>

#include <packeteer/connector.h>

namespace pk = packeteer;
namespace tc = twine::chrono;

namespace {

/**
 * Test callback. We're making it a member function so we can easily hand it
 * some parameters.
 **/
struct test_callback
{
  std::atomic<int>      m_called;
  std::atomic<uint64_t> m_mask;

  test_callback()
    : m_called(0)
    , m_mask(0)
  {
  }


  pk::error_t
  func(uint64_t mask, pk::error_t error, pk::handle const & h, void *)
  {
    ++m_called;
    m_mask = mask;
    LOG("callback called: " << error << " - " << h << " - " << mask);

    return pk::error_t(0);
  }
};



struct thread_id_callback
{
  twine::thread::id  m_tid;

  thread_id_callback()
    : m_tid()
  {
  }


  pk::error_t
  func(uint64_t, pk::error_t, pk::handle const &, void *)
  {
    m_tid = twine::this_thread::get_id();

    LOG("callback started");
    tc::sleep(tc::milliseconds(50));
    LOG("callback ended");

    return pk::error_t(0);
  }
};


/**
 * Help verify callback expectations.
 **/
#define ASSERT_CALLBACK(cb, expected_called, expected_mask) \
  {                                                         \
    int called = cb.m_called;                               \
    CPPUNIT_ASSERT_EQUAL(expected_called, called);          \
    uint64_t mask = cb.m_mask;                              \
    CPPUNIT_ASSERT_EQUAL(uint64_t(expected_mask), mask);    \
    cb.m_mask = 0; /* reset mask */                         \
  }

#define ASSERT_CALLBACK_GREATER(cb, expected_called, expected_mask) \
  {                                                         \
    int called = cb.m_called;                               \
    CPPUNIT_ASSERT(called > expected_called);               \
    uint64_t mask = cb.m_mask;                              \
    CPPUNIT_ASSERT_EQUAL(uint64_t(expected_mask), mask);    \
    cb.m_mask = 0; /* reset mask */                         \
  }

} // anonymous namespace

template <
  int SCHED_TYPE
>
class SchedulerTestImpl
    : public CppUnit::TestFixture
{
public:

  void testDelayedCallback()
  {
    // We only need one thread for this.
    pk::scheduler sched(1, static_cast<pk::scheduler::scheduler_type>(SCHED_TYPE));

    test_callback source;
    pk::callback cb = pk::make_callback(&source, &test_callback::func);

    sched.schedule_once(tc::milliseconds(50), cb);

    tc::sleep(tc::milliseconds(100));

    int called = source.m_called;
    CPPUNIT_ASSERT_EQUAL(1, called);

    uint64_t mask = source.m_mask;
    CPPUNIT_ASSERT_EQUAL(uint64_t(pk::PEV_TIMEOUT), mask);
  }



  void testTimedCallback()
  {
    // We only need one thread for this.
    pk::scheduler sched(1, static_cast<pk::scheduler::scheduler_type>(SCHED_TYPE));

    test_callback source;
    pk::callback cb = pk::make_callback(&source, &test_callback::func);

    sched.schedule_at(tc::now() + tc::milliseconds(50), cb);

    tc::sleep(tc::milliseconds(100));

    int called = source.m_called;
    CPPUNIT_ASSERT_EQUAL(1, called);

    uint64_t mask = source.m_mask;
    CPPUNIT_ASSERT_EQUAL(uint64_t(pk::PEV_TIMEOUT), mask);
  }



  void testRepeatCallback()
  {
    // We only need one thread for this.
    pk::scheduler sched(1, static_cast<pk::scheduler::scheduler_type>(SCHED_TYPE));

    test_callback source;
    pk::callback cb = pk::make_callback(&source, &test_callback::func);

    sched.schedule(tc::milliseconds(0), tc::milliseconds(50),
        3, cb);

    tc::sleep(tc::milliseconds(200));

    int called = source.m_called;
    CPPUNIT_ASSERT_EQUAL(3, called);

    uint64_t mask = source.m_mask;
    CPPUNIT_ASSERT_EQUAL(uint64_t(pk::PEV_TIMEOUT), mask);
  }



  void testInfiniteCallback()
  {
    // Infinite callbacks are easy enough to test for in that the callback
    // must have been invoked more than once just as above. However, once
    // explicitly unscheduled, the callback cannot be invoked any longer.

    // We only need one thread for this.
    pk::scheduler sched(1, static_cast<pk::scheduler::scheduler_type>(SCHED_TYPE));

    test_callback source;
    pk::callback cb = pk::make_callback(&source, &test_callback::func);

    sched.schedule(tc::milliseconds(0), tc::milliseconds(50), cb);

    // Since the first invocation happens immediately, we want to sleep <
    // 3 * 50 msec
    tc::sleep(tc::milliseconds(125));

    int called = source.m_called;
    CPPUNIT_ASSERT_EQUAL(3, called);

    uint64_t mask = source.m_mask;
    CPPUNIT_ASSERT_EQUAL(uint64_t(pk::PEV_TIMEOUT), mask);

    sched.unschedule(cb);

    tc::sleep(tc::milliseconds(100));

    // The amount of invocations may not have changed after the unschedule()
    // call above, even though we waited longer.
    called = source.m_called;
    CPPUNIT_ASSERT_EQUAL(3, called);

    mask = source.m_mask;
    CPPUNIT_ASSERT_EQUAL(uint64_t(pk::PEV_TIMEOUT), mask);
  }



  void testDelayedRepeatCallback()
  {
    // Kind of tricky; in order to register the delay, we need to choose the
    // initial delay, the repeat interval, and the wait time such that without
    // the delay we'd have more repetitions at the end of the wait time than
    // with the delay.
    // That means the repeat interval needs to be just under half of the wait
    // time.
    tc::milliseconds wait = tc::milliseconds(200);
    tc::milliseconds interval = tc::milliseconds(80);
    // Now the initial delay needs to be just higher than the difference between
    // the wait time and two intervals, i.e. delay > wait - 2 * interval
    tc::milliseconds delay = tc::milliseconds(60);

    // We only need one thread for this.
    pk::scheduler sched(1, static_cast<pk::scheduler::scheduler_type>(SCHED_TYPE));

    test_callback source;
    pk::callback cb = pk::make_callback(&source, &test_callback::func);

    sched.schedule(delay, interval, -1, cb);

    tc::sleep(wait);

    // If called is 3 or more, the initial delay wasn't honored.
    int called = source.m_called;
    CPPUNIT_ASSERT_EQUAL(2, called);

    uint64_t mask = source.m_mask;
    CPPUNIT_ASSERT_EQUAL(uint64_t(pk::PEV_TIMEOUT), mask);

    sched.unschedule(cb);
  }



  void testParallelCallbacks()
  {
    // Test that callbacks are executed in parallel by scheduling two at the
    // same time, and using two worker threads. Each callback sleeps for a while
    // and remembers its thread id; the two callbacks need to have different
    // thread ids afterwards for this to succeed.

    // We need >1 thread to enable parallell processing.
    pk::scheduler sched(2, static_cast<pk::scheduler::scheduler_type>(SCHED_TYPE));

    thread_id_callback source1;
    pk::callback cb1 = pk::make_callback(&source1, &thread_id_callback::func);
    thread_id_callback source2;
    pk::callback cb2 = pk::make_callback(&source2, &thread_id_callback::func);

    sched.schedule_once(tc::milliseconds(50), cb1);
    sched.schedule_once(tc::milliseconds(50), cb2);

    tc::sleep(tc::milliseconds(150));

    twine::thread::id id1 = source1.m_tid;
    twine::thread::id id2 = source2.m_tid;
    CPPUNIT_ASSERT(id1 != id2);
  }



  void testUserCallback()
  {
    // We register the same callback for two user-defined events; firing either
    // must cause the callback to be invoked.
    // Unregistering the callback from one of the events must cause the callback
    // to only be invoked for the other.
    enum user_events
    {
      EVENT_1 = 1 * pk::PEV_USER,
      EVENT_2 = 2 * pk::PEV_USER,
      EVENT_3 = 4 * pk::PEV_USER,
    };

    // We only need one thread for this.
    pk::scheduler sched(1, static_cast<pk::scheduler::scheduler_type>(SCHED_TYPE));

    test_callback source1;
    pk::callback cb1 = pk::make_callback(&source1, &test_callback::func);
    sched.register_event(EVENT_1 | EVENT_2 | EVENT_3, cb1);

    test_callback source2;
    pk::callback cb2 = pk::make_callback(&source2, &test_callback::func);
    sched.register_event(EVENT_2 | EVENT_3, cb2);

    CPPUNIT_ASSERT(cb1 != cb2);
    CPPUNIT_ASSERT(cb1.hash() != cb2.hash());

    // EVENT_1
    sched.fire_events(EVENT_1);
    tc::sleep(tc::milliseconds(50));

    ASSERT_CALLBACK(source1, 1, EVENT_1);
    ASSERT_CALLBACK(source2, 0, 0);

    // EVENT_2
    sched.fire_events(EVENT_2);
    tc::sleep(tc::milliseconds(50));

    ASSERT_CALLBACK(source1, 2, EVENT_2);
    ASSERT_CALLBACK(source2, 1, EVENT_2);

    // EVENT_3
    sched.fire_events(EVENT_3);
    tc::sleep(tc::milliseconds(50));

    ASSERT_CALLBACK(source1, 3, EVENT_3);
    ASSERT_CALLBACK(source2, 2, EVENT_3);

    // EVENT_1 | EVENT_2
    sched.fire_events(EVENT_1 | EVENT_2);
    tc::sleep(tc::milliseconds(50));

    ASSERT_CALLBACK(source1, 4, EVENT_1 | EVENT_2);
    ASSERT_CALLBACK(source2, 3, EVENT_2);

    // EVENT_2 | EVENT_3
    sched.fire_events(EVENT_2 | EVENT_3);
    tc::sleep(tc::milliseconds(50));

    ASSERT_CALLBACK(source1, 5, EVENT_2 | EVENT_3);
    ASSERT_CALLBACK(source2, 4, EVENT_2 | EVENT_3);

    // EVENT_1 | EVENT_3
    sched.fire_events(EVENT_1 | EVENT_3);
    tc::sleep(tc::milliseconds(50));

    ASSERT_CALLBACK(source1, 6, EVENT_1 | EVENT_3);
    ASSERT_CALLBACK(source2, 5, EVENT_3);

    // Unregister one from EVENT_2
    sched.unregister_event(EVENT_2, cb1);

    // EVENT_1
    sched.fire_events(EVENT_1);
    tc::sleep(tc::milliseconds(50));

    ASSERT_CALLBACK(source1, 7, EVENT_1);
    ASSERT_CALLBACK(source2, 5, 0); // mask reset; not called

    // EVENT_3
    sched.fire_events(EVENT_2);
    tc::sleep(tc::milliseconds(50));

    ASSERT_CALLBACK(source1, 7, 0); // mask reset; not called
    ASSERT_CALLBACK(source2, 6, EVENT_2);

    // EVENT_3
    sched.fire_events(EVENT_3);
    tc::sleep(tc::milliseconds(50));

    ASSERT_CALLBACK(source1, 8, EVENT_3);
    ASSERT_CALLBACK(source2, 7, EVENT_3);

    // EVENT_1 | EVENT_2
    sched.fire_events(EVENT_1 | EVENT_2);
    tc::sleep(tc::milliseconds(50));

    ASSERT_CALLBACK(source1, 9, EVENT_1);
    ASSERT_CALLBACK(source2, 8, EVENT_2);

    // EVENT_2 | EVENT_3
    sched.fire_events(EVENT_2 | EVENT_3);
    tc::sleep(tc::milliseconds(50));

    ASSERT_CALLBACK(source1, 10, EVENT_3);
    ASSERT_CALLBACK(source2, 9, EVENT_2 | EVENT_3);

    // EVENT_1 | EVENT_3
    sched.fire_events(EVENT_1 | EVENT_3);
    tc::sleep(tc::milliseconds(50));

    ASSERT_CALLBACK(source1, 11, EVENT_1 | EVENT_3);
    ASSERT_CALLBACK(source2, 10, EVENT_3);


    // Also ensure that fire_event() does not work with system events.
    CPPUNIT_ASSERT_EQUAL(pk::ERR_INVALID_VALUE,
        sched.fire_events(pk::PEV_IO_READ));
  }



  void testIOCallback()
  {
    // The simplest way to test I/O callbacks is with a pipe.
    pk::connector pipe("anon://");
    pipe.connect();

    // We only need one thread for this.
    pk::scheduler sched(1, static_cast<pk::scheduler::scheduler_type>(SCHED_TYPE));

    test_callback source1;
    pk::callback cb1 = pk::make_callback(&source1, &test_callback::func);
    sched.register_handle(pk::PEV_IO_READ, pipe.get_read_handle(), cb1);

    test_callback source2;
    pk::callback cb2 = pk::make_callback(&source2, &test_callback::func);
    sched.register_handle(pk::PEV_IO_WRITE, pipe.get_write_handle(), cb2);

    tc::sleep(tc::milliseconds(50));

    sched.unregister_handle(pk::PEV_IO_WRITE, pipe.get_write_handle(), cb2);

    tc::sleep(tc::milliseconds(50));

    // The second callback must have been invoked multiple times, because the
    // pipe is always (at this level of I/O load) writeable.
    ASSERT_CALLBACK_GREATER(source2, 0, pk::PEV_IO_WRITE);

    // On the other hand, without writing to the pipe, we should not have any
    // callbacks for reading.
    ASSERT_CALLBACK(source1, 0, 0);

    // So let's write something to the pipe. This will trigger the read callback
    // until we're reading from the pipe again.
    char buf[] = { '\0' };
    size_t amount = 0;
    pipe.write(buf, sizeof(buf), amount);
    CPPUNIT_ASSERT_EQUAL(sizeof(buf), amount);

    tc::sleep(tc::milliseconds(50));

    ASSERT_CALLBACK_GREATER(source1, 0, pk::PEV_IO_READ);

    int current = source1.m_called;

    pipe.read(buf, sizeof(buf), amount);
    CPPUNIT_ASSERT_EQUAL(sizeof(buf), amount);

    // Now the callbacks should stop again - we'll check by ensuring we don't
    // have more than 'current' invocations even after a delay. However, timing
    // being a bit of a bitch, we'll have to make some allowances.
    tc::sleep(tc::milliseconds(100));

    ASSERT_CALLBACK_GREATER(source1, current, pk::PEV_IO_READ);
    current = source1.m_called - current;
    CPPUNIT_ASSERT_MESSAGE("Should not normally fail.", current < 3);
  }



  void testSingleThreaded()
  {
    // We use a single user-triggered event here for simplicity.
    enum user_events
    {
      EVENT_1 = 1 * pk::PEV_USER,
    };

    // Single-threaded scheduler
    pk::scheduler sched(0, static_cast<pk::scheduler::scheduler_type>(SCHED_TYPE));

    test_callback source1;
    pk::callback cb1 = pk::make_callback(&source1, &test_callback::func);
    sched.register_event(EVENT_1, cb1);

    // EVENT_1
    sched.fire_events(EVENT_1);
    sched.process_events(tc::milliseconds(20));

    ASSERT_CALLBACK(source1, 1, EVENT_1);
  }
};



#if defined(PACKETEER_HAVE_EPOLL_CREATE1)
class SchedulerTestEpoll
    : public SchedulerTestImpl<pk::scheduler::TYPE_EPOLL>
{
public:
  CPPUNIT_TEST_SUITE(SchedulerTestEpoll);

    // Scheduled callbacks
    CPPUNIT_TEST(testDelayedCallback);
    CPPUNIT_TEST(testTimedCallback);
    CPPUNIT_TEST(testRepeatCallback);
    CPPUNIT_TEST(testInfiniteCallback);
    CPPUNIT_TEST(testDelayedRepeatCallback);
    CPPUNIT_TEST(testParallelCallbacks);

    // User callbacks
    CPPUNIT_TEST(testUserCallback);

    // I/O callbacks
    CPPUNIT_TEST(testIOCallback);

    // Single-threaded operation
    CPPUNIT_TEST(testSingleThreaded);

  CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(SchedulerTestEpoll);
#endif



#if defined(PACKETEER_HAVE_POLL)
class SchedulerTestPoll
    : public SchedulerTestImpl<pk::scheduler::TYPE_POLL>
{
public:
  CPPUNIT_TEST_SUITE(SchedulerTestPoll);

    // Scheduled callbacks
    CPPUNIT_TEST(testDelayedCallback);
    CPPUNIT_TEST(testTimedCallback);
    CPPUNIT_TEST(testRepeatCallback);
    CPPUNIT_TEST(testInfiniteCallback);
    CPPUNIT_TEST(testDelayedRepeatCallback);
    CPPUNIT_TEST(testParallelCallbacks);

    // User callbacks
    CPPUNIT_TEST(testUserCallback);

    // I/O callbacks
    CPPUNIT_TEST(testIOCallback);

    // Single-threaded operation
    CPPUNIT_TEST(testSingleThreaded);

  CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(SchedulerTestPoll);
#endif



#if defined(PACKETEER_HAVE_SELECT)
class SchedulerTestSelect
    : public SchedulerTestImpl<pk::scheduler::TYPE_SELECT>
{
public:
  CPPUNIT_TEST_SUITE(SchedulerTestSelect);

    // Scheduled callbacks
    CPPUNIT_TEST(testDelayedCallback);
    CPPUNIT_TEST(testTimedCallback);
    CPPUNIT_TEST(testRepeatCallback);
    CPPUNIT_TEST(testInfiniteCallback);
    CPPUNIT_TEST(testDelayedRepeatCallback);
    CPPUNIT_TEST(testParallelCallbacks);

    // User callbacks
    CPPUNIT_TEST(testUserCallback);

    // I/O callbacks
    CPPUNIT_TEST(testIOCallback);

    // Single-threaded operation
    CPPUNIT_TEST(testSingleThreaded);

  CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(SchedulerTestSelect);
#endif



#if defined(PACKETEER_HAVE_KQUEUE)
class SchedulerTestKQueue
    : public SchedulerTestImpl<pk::scheduler::TYPE_KQUEUE>
{
public:
  CPPUNIT_TEST_SUITE(SchedulerTestKQueue);

    // Scheduled callbacks
    CPPUNIT_TEST(testDelayedCallback);
    CPPUNIT_TEST(testTimedCallback);
    CPPUNIT_TEST(testRepeatCallback);
    CPPUNIT_TEST(testInfiniteCallback);
    CPPUNIT_TEST(testDelayedRepeatCallback);
    CPPUNIT_TEST(testParallelCallbacks);

    // User callbacks
    CPPUNIT_TEST(testUserCallback);

    // I/O callbacks
    CPPUNIT_TEST(testIOCallback);

    // Single-threaded operation
    CPPUNIT_TEST(testSingleThreaded);

  CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(SchedulerTestKQueue);
#endif
