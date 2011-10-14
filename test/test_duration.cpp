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

#include <packetflinger/duration.h>

#include <cppunit/extensions/HelperMacros.h>

#include <boost/thread.hpp>

#include <unistd.h>



namespace pd = packetflinger::duration;

namespace {

void thread_sleep_test()
{
  int32_t start = pd::to_sec(pd::now());
  pd::sleep(pd::from_sec(1));
  CPPUNIT_ASSERT_EQUAL(int32_t(1), pd::to_sec(pd::now()) - start);
}

} // anonymous namespace


class DurationTest
    : public CppUnit::TestFixture
{
public:
  CPPUNIT_TEST_SUITE(DurationTest);

    CPPUNIT_TEST(testConversion);
    CPPUNIT_TEST(testNow);
    CPPUNIT_TEST(testSleep);
    CPPUNIT_TEST(testSleepThreaded);

  CPPUNIT_TEST_SUITE_END();

private:

  void testConversion()
  {
    // Test conversion functions in time.h
    CPPUNIT_ASSERT_EQUAL(int32_t(1), pd::to_sec(1234567));
    CPPUNIT_ASSERT_EQUAL(int32_t(1234), pd::to_msec(1234567));
    CPPUNIT_ASSERT_EQUAL(pd::usec_t(1000000), pd::from_sec(1));
    CPPUNIT_ASSERT_EQUAL(pd::usec_t(1000), pd::from_msec(1));
  }



  void testNow()
  {
    // Test that packetflinger::time::now() returns the correct time.
    int32_t start = pd::to_sec(pd::now());
    ::sleep(1);
    CPPUNIT_ASSERT_EQUAL(int32_t(1), pd::to_sec(pd::now()) - start);
  }



  void testSleep()
  {
    // Test time::sleep() function - same test as testNow(), but using our own
    // sleep.
    int32_t start = pd::to_sec(pd::now());
    pd::sleep(pd::from_sec(1));
    CPPUNIT_ASSERT_EQUAL(int32_t(1), pd::to_sec(pd::now()) - start);
  }



  void testSleepThreaded()
  {
    // Test time::sleep() function only affects the current thread.
    int32_t start = pd::to_sec(pd::now());

    boost::thread th(&thread_sleep_test);

    CPPUNIT_ASSERT_EQUAL(int32_t(0), pd::to_sec(pd::now()) - start);

    th.join();

    CPPUNIT_ASSERT_EQUAL(int32_t(1), pd::to_sec(pd::now()) - start);
  }
};


CPPUNIT_TEST_SUITE_REGISTRATION(DurationTest);
