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
#include <packetflinger/detail/scheduler_impl.h>

#include <cppunit/extensions/HelperMacros.h>

#include <utility>

#include <boost/lambda/lambda.hpp>

namespace pf = packetflinger;

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

    CPPUNIT_TEST(testScheduledCallbacksContainer);

  CPPUNIT_TEST_SUITE_END();

private:


  void testIOCallbacksContainer()
  {
    // TODO
    // Lastly, we want to be able to find a range of eventmasks for a given
    // file descriptor. Since event masks are bitfields, and the index is
    // ordered, we should be able to find candidates quicker because we know
    // the event masks we're looking for will be >= the event that got
    // triggered.
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

    auto & timeout_index = container.get<pf::detail::timeout_tag>();

    pf::detail::scheduled_callback_entry * entry = new pf::detail::scheduled_callback_entry();
    entry->m_timeout = 2;
    entry->m_callback = &foo;
    timeout_index.insert(entry);

    entry = new pf::detail::scheduled_callback_entry();
    entry->m_timeout = 3;
    entry->m_callback = &bar;
    timeout_index.insert(entry);

    entry = new pf::detail::scheduled_callback_entry();
    entry->m_timeout = 1;
    entry->m_callback = &foo;
    timeout_index.insert(entry);

    CPPUNIT_ASSERT_EQUAL(size_t(3), timeout_index.size());

    pf::duration::usec_t prev = 0;
    for (auto value : timeout_index) {
      CPPUNIT_ASSERT(prev < value->m_timeout);
      prev = value->m_timeout;
    }

    // Alright, it's ordered. Now Grab the entry with timeout #2 and bump the
    // timeout value to 4. Then check if the ordering has changed (i.e. whether
    // it's still lowest to highest).
    auto it = timeout_index.find(2);
    CPPUNIT_ASSERT_EQUAL(pf::duration::usec_t(2), (*it)->m_timeout);
    timeout_index.modify_key(it, boost::lambda::_1 = 4);
    CPPUNIT_ASSERT_EQUAL(pf::duration::usec_t(4), (*it)->m_timeout);

    prev = 0;
    for (auto value : timeout_index) {
      CPPUNIT_ASSERT(prev < value->m_timeout);
      prev = value->m_timeout;
    }

    // Ok, resorting worked. Now let's find all entries with a timeout value
    // <= 3, and we'll have the functionality covered. We expect the timeout
    // values of the returned values to be 1 and 3, now that 2 has been bumped
    // to 4.
    auto range = container.range(0 <= boost::lambda::_1, boost::lambda::_1 <= 3);

    auto cit = range.first;
    CPPUNIT_ASSERT(range.second != cit);
    CPPUNIT_ASSERT_EQUAL(pf::duration::usec_t(1), (*cit)->m_timeout);

    ++cit;
    CPPUNIT_ASSERT(range.second != cit);
    CPPUNIT_ASSERT_EQUAL(pf::duration::usec_t(3), (*cit)->m_timeout);

    ++cit;
    CPPUNIT_ASSERT(range.second == cit);

    // Lastly, ensure that we can still find a specific callback's entry via
    // the callback pointer itself. Only one of the entries uses bar() as the
    // callback.
    auto & callback_index = container.get<packetflinger::detail::callback_tag>();
    auto cb_range = callback_index.equal_range(&bar);
    CPPUNIT_ASSERT(cb_range.first != cb_range.second);
  }

};

CPPUNIT_TEST_SUITE_REGISTRATION(SchedulerImplTest);
