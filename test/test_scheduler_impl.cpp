/**
 * This file is part of packetflinger.
 *
 * Author(s): Jens Finkhaeuser <jens@unwesen.co.uk>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 * Copyright (c) 2012 Unwesen Ltd.
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
#include <packetflinger/detail/scheduler_impl.h>

#include <cppunit/extensions/HelperMacros.h>

#include <utility>

#include <boost/lambda/lambda.hpp>

namespace pf = packetflinger;
namespace tc = twine::chrono;

namespace {

/**
 * Two callbacks, just so we have different function pointers to work with.
 **/
pf::error_t
foo(uint64_t mask, pf::error_t error, int fd, void * baton)
{
  // no-op
}

pf::error_t
bar(uint64_t mask, pf::error_t error, int fd, void * baton)
{
  // no-op
}

pf::error_t
baz(uint64_t mask, pf::error_t error, int fd, void * baton)
{
  // no-op
}

} // anonymous namespace

/**
 * We don't really test all of scheduler_impl here, just the containers that
 * the implementation uses. They form much of the basis of how the scheduler
 * interacts with implementation (epoll, kqueue, etc.) specific parts.
 * SchedulerTest then performs something closer to integration tests between
 * the implementation-specific and the generic parts.
 **/
class SchedulerImplTest
    : public CppUnit::TestFixture
{
public:
  CPPUNIT_TEST_SUITE(SchedulerImplTest);

    CPPUNIT_TEST(testIOCallbacksContainer);
    CPPUNIT_TEST(testScheduledCallbacksContainer);
    CPPUNIT_TEST(testUserCallbacksContainer);

  CPPUNIT_TEST_SUITE_END();

private:

  void testIOCallbacksContainer()
  {
    // Lastly, we want to be able to find a range of eventmasks for a given
    // file descriptor. Since event masks are bitfields, and the index is
    // ordered, we should be able to find candidates quicker because we know
    // the event masks we're looking for will be >= the event that got
    // triggered.
    CPPUNIT_FAIL("not yet implemented");
  }



  void testScheduledCallbacksContainer()
  {
    // Ensure that constraints imposed on the container for scheduled callbacks
    // fulfil the requirements.

    // First, add three callbacks (we can ignore the callback function itself)
    // at two different m_timeout values. If the container works as intended,
    // the callback with the lowest timeout value will be found first on
    // iteration.
    pf::detail::scheduled_callbacks_t container;

    pf::detail::scheduled_callback_entry * entry = new pf::detail::scheduled_callback_entry();
    entry->m_timeout = tc::microseconds(2);
    entry->m_callback = &foo;
    container.add(entry);

    entry = new pf::detail::scheduled_callback_entry();
    entry->m_timeout = tc::microseconds(3);
    entry->m_callback = &bar;
    container.add(entry);

    entry = new pf::detail::scheduled_callback_entry();
    entry->m_timeout = tc::microseconds(1);
    entry->m_callback = &foo;
    container.add(entry);

    entry = new pf::detail::scheduled_callback_entry();
    entry->m_timeout = tc::microseconds(3);
    entry->m_callback = &baz;
    container.add(entry);

    auto timeout_index = container.get_timed_out(twine::chrono::microseconds(0));
    CPPUNIT_ASSERT_EQUAL(size_t(0), timeout_index.size());
    timeout_index = container.get_timed_out(twine::chrono::microseconds(2));
    CPPUNIT_ASSERT_EQUAL(size_t(2), timeout_index.size());
    timeout_index = container.get_timed_out(twine::chrono::microseconds(3));
    CPPUNIT_ASSERT_EQUAL(size_t(4), timeout_index.size());

    tc::microseconds prev = tc::microseconds(0);
    for (auto value : timeout_index) {
      CPPUNIT_ASSERT(prev <= value->m_timeout);
      prev = value->m_timeout;
    }

    // Ensure that when we remove an entry, that's reflected in the timeout index
    entry = new pf::detail::scheduled_callback_entry();
    entry->m_timeout = tc::microseconds(2);
    entry->m_callback = &foo;
    container.remove(entry);

    timeout_index = container.get_timed_out(twine::chrono::microseconds(0));
    CPPUNIT_ASSERT_EQUAL(size_t(0), timeout_index.size());
    timeout_index = container.get_timed_out(twine::chrono::microseconds(3));
    CPPUNIT_ASSERT_EQUAL(size_t(3), timeout_index.size());

    prev = tc::microseconds(0);
    for (auto value : timeout_index) {
      CPPUNIT_ASSERT(prev <= value->m_timeout);
      prev = value->m_timeout;
    }
  }



  void testUserCallbacksContainer()
  {
    CPPUNIT_FAIL("not yet implemented");
/*
    // The user callbacks container needs to fulfil two criteria. The simpler
    // one is that callbacks need to be found via a specific index. The trickier
    // one is that event masks need to be matched reasonably quickly, which
    // means finding entries with events >= a given event mask.
    enum user_events
    {
      EVENT_1 = 1 * pf::scheduler::EV_USER,
      EVENT_2 = 2 * pf::scheduler::EV_USER,
      EVENT_3 = 4 * pf::scheduler::EV_USER,
    };

    pf::detail::user_callbacks_t container;

    auto & callback_index = container.get<pf::detail::callback_tag>();

    pf::detail::user_callback_entry * entry = new pf::detail::user_callback_entry();
    entry->m_events = EVENT_1;
    entry->m_callback = &foo;
    callback_index.insert(entry);

    entry = new pf::detail::user_callback_entry();
    entry->m_events = EVENT_3;
    entry->m_callback = &bar;
    callback_index.insert(entry);

    entry = new pf::detail::user_callback_entry();
    entry->m_events = EVENT_1 | EVENT_2;
    entry->m_callback = &foo;
    callback_index.insert(entry);

    // Finding entries for the 'foo' callback should yield 2 entries.
    auto range = callback_index.equal_range(&foo);
    CPPUNIT_ASSERT(range.first != callback_index.end());

    int count = 0;
    for (auto iter = range.first ; iter != range.second ; ++iter) {
      ++count;
    }
    CPPUNIT_ASSERT_EQUAL(2, count);
  */
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(SchedulerImplTest);
