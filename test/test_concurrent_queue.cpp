/**
 * This file is part of packetflinger.
 *
 * Author(s): Jens Finkhaeuser <unwesen@users.sourceforge.net>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
