/**
 * This file is part of packetflinger.
 *
 * Author(s): Jens Finkhaeuser <jens@unwesen.co.uk>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 * Copyright (c) 2012-2014 Unwesen Ltd.
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
#include <packetflinger/concurrent_queue.h>

#include <cppunit/extensions/HelperMacros.h>

namespace pf = packetflinger;


class ConcurrentQueueTest
    : public CppUnit::TestFixture
{
public:
  CPPUNIT_TEST_SUITE(ConcurrentQueueTest);

    CPPUNIT_TEST(testBasicFunctionality);

  CPPUNIT_TEST_SUITE_END();

private:

  void testBasicFunctionality()
  {
    // Test that the FIFO principle works for the queue as such, and that
    // extra functions do what they're supposed to do.
    pf::concurrent_queue<int> queue;
    CPPUNIT_ASSERT_EQUAL(true, queue.empty());
    CPPUNIT_ASSERT_EQUAL(size_t(0), queue.size());

    queue.push(42);
    CPPUNIT_ASSERT_EQUAL(false, queue.empty());
    CPPUNIT_ASSERT_EQUAL(size_t(1), queue.size());

    queue.push(666);
    CPPUNIT_ASSERT_EQUAL(false, queue.empty());
    CPPUNIT_ASSERT_EQUAL(size_t(2), queue.size());

    int value = 0;
    CPPUNIT_ASSERT(queue.pop(value));
    CPPUNIT_ASSERT_EQUAL(42, value);
    CPPUNIT_ASSERT_EQUAL(false, queue.empty());
    CPPUNIT_ASSERT_EQUAL(size_t(1), queue.size());

    CPPUNIT_ASSERT(queue.pop(value));
    CPPUNIT_ASSERT_EQUAL(666, value);
    CPPUNIT_ASSERT_EQUAL(true, queue.empty());
    CPPUNIT_ASSERT_EQUAL(size_t(0), queue.size());

    CPPUNIT_ASSERT(!queue.pop(value));
  }
};


CPPUNIT_TEST_SUITE_REGISTRATION(ConcurrentQueueTest);
