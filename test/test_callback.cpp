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
    CPPUNIT_TEST(testEmpty);
    CPPUNIT_TEST(testAssignment);

  CPPUNIT_TEST_SUITE_END();

private:
  void testFreeFunctions()
  {
    // Test that a free function is correctly invoked.
    pf::callback cb1 = &free_func1;
    CPPUNIT_ASSERT_EQUAL(pf::error_t(1), cb1(42, pf::error_t(0), 0, nullptr));

    pf::callback cb2 = &free_func2;
    CPPUNIT_ASSERT_EQUAL(pf::error_t(2), cb2(666, pf::error_t(0), 0, nullptr));

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
    CPPUNIT_ASSERT_EQUAL(pf::error_t(3), cb1(1234, pf::error_t(0), 0, nullptr));

    pf::callback cb2 = pf::make_callback(&f);
    CPPUNIT_ASSERT_EQUAL(pf::error_t(4), cb2(0xdeadbeef, pf::error_t(0), 0, nullptr));

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


  void testEmpty()
  {
    // Empty/un-assigned callbacks should behave sanely
    pf::callback cb;

    CPPUNIT_ASSERT_EQUAL(true, cb.empty());
    CPPUNIT_ASSERT(!cb);

    CPPUNIT_ASSERT_THROW(cb(0, pf::error_t(1), 2, nullptr), pf::exception);

    pf::callback cb2 = &free_func1;
    CPPUNIT_ASSERT(cb != cb2);
  }


  void testAssignment()
  {
    // Ensure that empty callbacks can be assigned later on.
    pf::callback cb;
    CPPUNIT_ASSERT(!cb);

    cb = &free_func1;
    CPPUNIT_ASSERT(cb);
    CPPUNIT_ASSERT_EQUAL(false, cb.empty());
    CPPUNIT_ASSERT_EQUAL(pf::error_t(1), cb(42, pf::error_t(0), 0, nullptr));

    functor f;
    cb = pf::make_callback(&f);
    CPPUNIT_ASSERT(cb);
    CPPUNIT_ASSERT_EQUAL(false, cb.empty());
    CPPUNIT_ASSERT_EQUAL(pf::error_t(4), cb(0xdeadbeef, pf::error_t(0), 0, nullptr));
  }
};


CPPUNIT_TEST_SUITE_REGISTRATION(CallbackTest);
