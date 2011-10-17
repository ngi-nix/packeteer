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

#include <packetflinger/callback.h>

#include <cppunit/extensions/HelperMacros.h>

namespace pf = packetflinger;

namespace {

pf::error_t free_func1(uint64_t events, pf::error_t error, int fd, void * baton)
{
  CPPUNIT_ASSERT_EQUAL(uint64_t(42), events);
  return pf::error_t(1);
}


pf::error_t free_func2(uint64_t events, pf::error_t error, int fd, void * baton)
{
  CPPUNIT_ASSERT_EQUAL(uint64_t(666), events);
  return pf::error_t(2);
}


struct functor
{
  pf::error_t member_func(uint64_t events, pf::error_t error, int fd, void * baton)
  {
    CPPUNIT_ASSERT_EQUAL(uint64_t(1234), events);
    return pf::error_t(3);
  }



  pf::error_t operator()(uint64_t events, pf::error_t error, int fd, void * baton)
  {
    CPPUNIT_ASSERT_EQUAL(uint64_t(0xdeadbeef), events);
    return pf::error_t(4);
  }
};


} // anonymous namespace


class CallbackTest
    : public CppUnit::TestFixture
{
public:
  CPPUNIT_TEST_SUITE(CallbackTest);

    CPPUNIT_TEST(testFreeFunctions);
    CPPUNIT_TEST(testMemberFunctions);
    CPPUNIT_TEST(testComparison);

  CPPUNIT_TEST_SUITE_END();

private:
  void testFreeFunctions()
  {
    // Test that a free function is correctly invoked.
    pf::callback cb1 = &free_func1;
    CPPUNIT_ASSERT_EQUAL(pf::error_t(1), cb1(42, pf::error_t(0), 0, NULL));

    pf::callback cb2 = &free_func2;
    CPPUNIT_ASSERT_EQUAL(pf::error_t(2), cb2(666, pf::error_t(0), 0, NULL));

    // Test for equality.
    CPPUNIT_ASSERT(cb1 != cb2);
    pf::callback cb3 = &free_func1;
    CPPUNIT_ASSERT(cb1 == cb3);
  }



  void testMemberFunctions()
  {
    // Test that member functions are correctly invoked.
    functor f;

    pf::callback cb1 = pf::make_callback(&f, &functor::member_func);
    CPPUNIT_ASSERT_EQUAL(pf::error_t(3), cb1(1234, pf::error_t(0), 0, NULL));

    pf::callback cb2 = pf::make_callback(&f);
    CPPUNIT_ASSERT_EQUAL(pf::error_t(4), cb2(0xdeadbeef, pf::error_t(0), 0, NULL));

    // Test for equality.
    CPPUNIT_ASSERT(cb1 != cb2);
    pf::callback cb3 = pf::make_callback(&f, &functor::member_func);
    CPPUNIT_ASSERT(cb1 == cb3);
  }



  void testComparison()
  {
    // Test that a functor and a free function bound to callbacks do not
    // compare equal.
    functor f;

    pf::callback cb1 = pf::make_callback(&f, &functor::member_func);
    pf::callback cb2 = &free_func1;

    CPPUNIT_ASSERT(cb1 != cb2);
    CPPUNIT_ASSERT(cb2 != cb1);
  }
};


CPPUNIT_TEST_SUITE_REGISTRATION(CallbackTest);
