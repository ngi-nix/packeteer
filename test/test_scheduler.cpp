/**
 * This file is part of packetflinger.
 *
 * Author(s): Jens Finkhaeuser <unwesen@users.sourceforge.net>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
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
#include <packetflinger/scheduler.h>

#include <cppunit/extensions/HelperMacros.h>

#include <utility>
#include <atomic>

#include <boost/thread.hpp>

namespace pf = packetflinger;

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


  pf::error_t
  func(uint64_t mask, pf::error_t error, int fd, void * baton)
  {
    ++m_called;
    m_mask = mask;
    LOG("callback called");

    return pf::error_t(0);
  }
};



struct thread_id_callback
{
  boost::thread::id  m_tid;

  thread_id_callback()
    : m_tid()
  {
  }


  pf::error_t
  func(uint64_t mask, pf::error_t error, int fd, void * baton)
  {
    m_tid = boost::this_thread::get_id();

    LOG("callback started");
    pf::duration::sleep(pf::duration::from_msec(50));
    LOG("callback ended");

    return pf::error_t(0);
  }
};


} // anonymous namespace

class SchedulerTest
    : public CppUnit::TestFixture
{
public:
  CPPUNIT_TEST_SUITE(SchedulerTest);

    // Scheduled callbacks
    CPPUNIT_TEST(testDelayedCallback);
    CPPUNIT_TEST(testTimedCallback);
    CPPUNIT_TEST(testRepeatCallback);
    CPPUNIT_TEST(testInfiniteCallback);
    CPPUNIT_TEST(testDelayedRepeatCallback);
    CPPUNIT_TEST(testParallelCallbacks);

    // User callbacks
    CPPUNIT_TEST(testUserCallback);

  CPPUNIT_TEST_SUITE_END();

private:

  void testDelayedCallback()
  {
    pf::scheduler sched(1); // We only need one thread for this.

    test_callback source;
    pf::callback cb = pf::make_callback(&source, &test_callback::func);

    sched.schedule_once(pf::duration::from_msec(50), cb);

    pf::duration::sleep(pf::duration::from_msec(100));

    int called = source.m_called;
    CPPUNIT_ASSERT_EQUAL(1, called);

    uint64_t mask = source.m_mask;
    CPPUNIT_ASSERT_EQUAL((uint64_t) pf::scheduler::EV_TIMEOUT, mask);
  }



  void testTimedCallback()
  {
    pf::scheduler sched(1); // We only need one thread for this.

    test_callback source;
    pf::callback cb = pf::make_callback(&source, &test_callback::func);

    sched.schedule_at(pf::duration::now() + pf::duration::from_msec(50), cb);

    pf::duration::sleep(pf::duration::from_msec(100));

    int called = source.m_called;
    CPPUNIT_ASSERT_EQUAL(1, called);

    uint64_t mask = source.m_mask;
    CPPUNIT_ASSERT_EQUAL((uint64_t) pf::scheduler::EV_TIMEOUT, mask);
  }



  void testRepeatCallback()
  {
    pf::scheduler sched(1); // We only need one thread for this.

    test_callback source;
    pf::callback cb = pf::make_callback(&source, &test_callback::func);

    sched.schedule(pf::duration::usec_t(0), pf::duration::from_msec(50),
        3, cb);

    pf::duration::sleep(pf::duration::from_msec(200));

    int called = source.m_called;
    CPPUNIT_ASSERT_EQUAL(3, called);

    uint64_t mask = source.m_mask;
    CPPUNIT_ASSERT_EQUAL((uint64_t) pf::scheduler::EV_TIMEOUT, mask);
  }



  void testInfiniteCallback()
  {
    // Infinite callbacks are easy enough to test for in that the callback
    // must have been invoked more than once just as above. However, once
    // explicitly unscheduled, the callback cannot be invoked any longer.
    pf::scheduler sched(1); // We only need one thread for this.

    test_callback source;
    pf::callback cb = pf::make_callback(&source, &test_callback::func);

    sched.schedule(pf::duration::usec_t(0), pf::duration::from_msec(50), cb);

    // Since the first invocation happens immediately, we want to sleep <
    // 3 * 50 msec
    pf::duration::sleep(pf::duration::from_msec(125));

    int called = source.m_called;
    CPPUNIT_ASSERT_EQUAL(3, called);

    uint64_t mask = source.m_mask;
    CPPUNIT_ASSERT_EQUAL((uint64_t) pf::scheduler::EV_TIMEOUT, mask);

    sched.unschedule(cb);

    pf::duration::sleep(pf::duration::from_msec(100));

    // The amount of invocations may not have changed after the unschedule()
    // call above, even though we waited longer.
    called = source.m_called;
    CPPUNIT_ASSERT_EQUAL(3, called);

    mask = source.m_mask;
    CPPUNIT_ASSERT_EQUAL((uint64_t) pf::scheduler::EV_TIMEOUT, mask);
  }



  void testDelayedRepeatCallback()
  {
    // Kind of tricky; in order to register the delay, we need to choose the
    // initial delay, the repeat interval, and the wait time such that without
    // the delay we'd have more repetitions at the end of the wait time than
    // with the delay.
    // That means the repeat interval needs to be just under half of the wait
    // time.
    pf::duration::usec_t wait = pf::duration::from_msec(200);
    pf::duration::usec_t interval = pf::duration::from_msec(80);
    // Now the initial delay needs to be just higher than the difference between
    // the wait time and two intervals, i.e. delay > wait - 2 * interval
    pf::duration::usec_t delay = pf::duration::from_msec(50);


    pf::scheduler sched(1); // We only need one thread for this.

    test_callback source;
    pf::callback cb = pf::make_callback(&source, &test_callback::func);

    sched.schedule(delay, interval, -1, cb);

    pf::duration::sleep(wait);

    // If called is 3 or more, the initial delay wasn't honored.
    int called = source.m_called;
    CPPUNIT_ASSERT_EQUAL(2, called);

    uint64_t mask = source.m_mask;
    CPPUNIT_ASSERT_EQUAL((uint64_t) pf::scheduler::EV_TIMEOUT, mask);

    sched.unschedule(cb);
  }



  void testParallelCallbacks()
  {
    // Test that callbacks are executed in parallel by scheduling two at the
    // same time, and using two worker threads. Each callback sleeps for a while
    // and remembers its thread id; the two callbacks need to have different
    // thread ids afterwards for this to succeed.
    pf::scheduler sched(2);

    thread_id_callback source1;
    pf::callback cb1 = pf::make_callback(&source1, &thread_id_callback::func);
    thread_id_callback source2;
    pf::callback cb2 = pf::make_callback(&source2, &thread_id_callback::func);

    sched.schedule_once(pf::duration::from_msec(50), cb1);
    sched.schedule_once(pf::duration::from_msec(50), cb2);

    pf::duration::sleep(pf::duration::from_msec(150));

    boost::thread::id id1 = source1.m_tid;
    boost::thread::id id2 = source2.m_tid;
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
      EVENT_1 = 1 * pf::scheduler::EV_USER,
      EVENT_2 = 2 * pf::scheduler::EV_USER,
    };

    pf::scheduler sched(1); // We only need one thread for this.

    test_callback source;
    pf::callback cb = pf::make_callback(&source, &test_callback::func);

    sched.register_event(EVENT_1 | EVENT_2, cb);

    sched.fire_events(EVENT_1);
    pf::duration::sleep(pf::duration::from_msec(50));

    int called = source.m_called;
    CPPUNIT_ASSERT_EQUAL(1, called);
    uint64_t mask = source.m_mask;
    CPPUNIT_ASSERT_EQUAL((uint64_t) EVENT_1, mask);

    sched.fire_events(EVENT_2);
    pf::duration::sleep(pf::duration::from_msec(50));

    called = source.m_called;
    CPPUNIT_ASSERT_EQUAL(2, called);
    mask = source.m_mask;
    CPPUNIT_ASSERT_EQUAL((uint64_t) EVENT_2, mask);

    sched.fire_events(EVENT_1 | EVENT_2);
    pf::duration::sleep(pf::duration::from_msec(50));

    called = source.m_called;
    CPPUNIT_ASSERT_EQUAL(3, called);
    mask = source.m_mask;
    CPPUNIT_ASSERT_EQUAL((uint64_t) EVENT_1 | EVENT_2, mask);

    sched.unregister_event(EVENT_2, cb);

    sched.fire_events(EVENT_1);
    pf::duration::sleep(pf::duration::from_msec(50));

    called = source.m_called;
    CPPUNIT_ASSERT_EQUAL(4, called);
    mask = source.m_mask;
    CPPUNIT_ASSERT_EQUAL((uint64_t) EVENT_1, mask);

    sched.fire_events(EVENT_2);
    pf::duration::sleep(pf::duration::from_msec(50));

    called = source.m_called;
    CPPUNIT_ASSERT_EQUAL(4, called); // not incremented.

    // Also ensure that fire_event() does not work with system events.
    CPPUNIT_ASSERT_EQUAL(pf::ERR_INVALID_VALUE,
        sched.fire_events(pf::scheduler::EV_IO_READ));
  }




};

CPPUNIT_TEST_SUITE_REGISTRATION(SchedulerTest);
