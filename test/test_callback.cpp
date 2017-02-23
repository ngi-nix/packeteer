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
#include <packeteer/callback.h>

#include <cppunit/extensions/HelperMacros.h>

#include <functional>

namespace pk = packeteer;

namespace {

pk::error_t free_func1(uint64_t events, pk::error_t, int, void *)
{
  CPPUNIT_ASSERT_EQUAL(uint64_t(42), events);
  return pk::error_t(1);
}


pk::error_t free_func2(uint64_t events, pk::error_t, int, void *)
{
  CPPUNIT_ASSERT_EQUAL(uint64_t(666), events);
  return pk::error_t(2);
}


struct functor
{
  pk::error_t member_func(uint64_t events, pk::error_t, int, void *)
  {
    CPPUNIT_ASSERT_EQUAL(uint64_t(1234), events);
    return pk::error_t(3);
  }



  pk::error_t operator()(uint64_t events, pk::error_t, int, void *)
  {
    CPPUNIT_ASSERT_EQUAL(uint64_t(0xdeadbeef), events);
    return pk::error_t(4);
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
    CPPUNIT_TEST(testHash);
    CPPUNIT_TEST(testCopy);

  CPPUNIT_TEST_SUITE_END();

private:
  void testFreeFunctions()
  {
    // Test that a free function is correctly invoked.
    pk::callback cb1 = &free_func1;
    CPPUNIT_ASSERT_EQUAL(pk::error_t(1), cb1(42, pk::error_t(0), 0, nullptr));

    pk::callback cb2 = &free_func2;
    CPPUNIT_ASSERT_EQUAL(pk::error_t(2), cb2(666, pk::error_t(0), 0, nullptr));

    // Test for equality.
    CPPUNIT_ASSERT(cb1 != cb2);
    pk::callback cb3 = &free_func1;
    CPPUNIT_ASSERT(cb1 == cb3);
  }



  void testMemberFunctions()
  {
    // Test that member functions are correctly invoked.
    functor f;

    pk::callback cb1 = pk::make_callback(&f, &functor::member_func);
    CPPUNIT_ASSERT_EQUAL(pk::error_t(3), cb1(1234, pk::error_t(0), 0, nullptr));

    pk::callback cb2 = pk::make_callback(&f);
    CPPUNIT_ASSERT_EQUAL(pk::error_t(4), cb2(0xdeadbeef, pk::error_t(0), 0, nullptr));

    // Test for equality.
    CPPUNIT_ASSERT(cb1 != cb2);
    pk::callback cb3 = pk::make_callback(&f, &functor::member_func);
    CPPUNIT_ASSERT(cb1 == cb3);
  }



  void testComparison()
  {
    // Test that a functor and a free function bound to callbacks do not
    // compare equal.
    functor f;

    pk::callback cb1 = pk::make_callback(&f, &functor::member_func);
    pk::callback cb2 = &free_func1;

    CPPUNIT_ASSERT(cb1 != cb2);
    CPPUNIT_ASSERT(cb2 != cb1);

    // Also check whether two callbacks encapsulating the same function/
    // functor compare equal.
    pk::callback cb3 = pk::make_callback(&f, &functor::member_func);
    CPPUNIT_ASSERT_EQUAL(cb1, cb3);

    pk::callback cb4 = &free_func1;
    CPPUNIT_ASSERT_EQUAL(cb2, cb4);

    // It's equally important that a callback constructed from a different
    // instance of the same functor class compares not equal.
    functor f2;
    pk::callback cb5 = pk::make_callback(&f2, &functor::member_func);
    CPPUNIT_ASSERT(cb1 != cb5);
    CPPUNIT_ASSERT(cb3 != cb5);
  }


  void testEmpty()
  {
    // Empty/un-assigned callbacks should behave sanely
    pk::callback cb;

    CPPUNIT_ASSERT_EQUAL(true, cb.empty());
    CPPUNIT_ASSERT(!cb);

    CPPUNIT_ASSERT_THROW(cb(0, pk::error_t(1), 2, nullptr), pk::exception);

    pk::callback cb2 = &free_func1;
    CPPUNIT_ASSERT(cb != cb2);
  }


  void testAssignment()
  {
    // Ensure that empty callbacks can be assigned later on.
    pk::callback cb;
    CPPUNIT_ASSERT(!cb);

    cb = &free_func1;
    CPPUNIT_ASSERT(cb);
    CPPUNIT_ASSERT_EQUAL(false, cb.empty());
    CPPUNIT_ASSERT_EQUAL(pk::error_t(1), cb(42, pk::error_t(0), 0, nullptr));

    functor f;
    cb = pk::make_callback(&f);
    CPPUNIT_ASSERT(cb);
    CPPUNIT_ASSERT_EQUAL(false, cb.empty());
    CPPUNIT_ASSERT_EQUAL(pk::error_t(4), cb(0xdeadbeef, pk::error_t(0), 0, nullptr));
  }


  void testHash()
  {
    PACKETEER_HASH_NAMESPACE ::hash<pk::callback> hasher;

    // Callbacks made from the same free function should have the same hash.
    pk::callback cb1 = &free_func1;
    pk::callback cb2 = &free_func1;
    CPPUNIT_ASSERT_EQUAL(hasher(cb1), hasher(cb2));

    // But they can't have the same hash as a callback made from a different
    // free function.
    pk::callback cb3 = &free_func2;
    CPPUNIT_ASSERT(hasher(cb1) != hasher(cb3));
    CPPUNIT_ASSERT(hasher(cb2) != hasher(cb3));

    // The equality constraint also applies to functors.
    functor f1;
    pk::callback cb4 = pk::make_callback(&f1, &functor::member_func);
    pk::callback cb5 = pk::make_callback(&f1, &functor::member_func);
    CPPUNIT_ASSERT_EQUAL(hasher(cb4), hasher(cb5));

    // And the same applies to the non-equality
    functor f2;
    pk::callback cb6 = pk::make_callback(&f2, &functor::member_func);
    CPPUNIT_ASSERT(hasher(cb4) != hasher(cb6));
    CPPUNIT_ASSERT(hasher(cb5) != hasher(cb6));
  }


  void testCopy()
  {
    // Copy ctor
    pk::callback cb1 = &free_func1;
    pk::callback cb2 = cb1;

    // Assign
    pk::callback cb3;
    cb3 = cb1;
  }
};


CPPUNIT_TEST_SUITE_REGISTRATION(CallbackTest);
