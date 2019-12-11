/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 * Copyright (c) 2012-2014 Unwesen Ltd.
 * Copyright (c) 2015-2019 Jens Finkhaeuser.
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
#include <packeteer/scheduler/callback.h>

#include <gtest/gtest.h>

#include <functional>

namespace pk = packeteer;

namespace {

pk::error_t free_func1(pk::events_t events, pk::error_t, pk::handle const &, void *)
{
  EXPECT_EQ(42, events);
  return pk::error_t(1);
}


pk::error_t free_func2(pk::events_t events, pk::error_t, pk::handle const &, void *)
{
  EXPECT_EQ(666, events);
  return pk::error_t(2);
}


struct functor
{
  pk::error_t member_func(pk::events_t events, pk::error_t, pk::handle const &, void *)
  {
    EXPECT_EQ(1234, events);
    return pk::error_t(3);
  }



  pk::error_t operator()(pk::events_t events, pk::error_t, pk::handle const &, void *)
  {
    EXPECT_EQ(0xdeadbeef, events);
    return pk::error_t(4);
  }
};


} // anonymous namespace


TEST(Callback, free_functions)
{
  // Test that a free function is correctly invoked.
  pk::callback cb1 = &free_func1;
  ASSERT_EQ(pk::error_t(1), cb1(42, pk::error_t(0), pk::handle::make_dummy(0), nullptr));

  pk::callback cb2 = &free_func2;
  ASSERT_EQ(pk::error_t(2), cb2(666, pk::error_t(0), pk::handle::make_dummy(0), nullptr));

  // Test for equality.
  ASSERT_NE(cb1, cb2);
  pk::callback cb3 = &free_func1;
  ASSERT_EQ(cb1, cb3);
}



TEST(Callback, member_functions)
{
  // Test that member functions are correctly invoked.
  functor f;

  pk::callback cb1 = pk::make_callback(&f, &functor::member_func);
  ASSERT_EQ(pk::error_t(3), cb1(1234, pk::error_t(0), pk::handle::make_dummy(0), nullptr));

  pk::callback cb2 = pk::make_callback(&f);
  ASSERT_EQ(pk::error_t(4), cb2(0xdeadbeef, pk::error_t(0), pk::handle::make_dummy(0), nullptr));

  // Test for equality.
  ASSERT_NE(cb1, cb2);
  pk::callback cb3 = pk::make_callback(&f, &functor::member_func);
  ASSERT_EQ(cb1, cb3);
}



TEST(Callback, comparison)
{
  // Test that a functor and a free function bound to callbacks do not
  // compare equal.
  functor f;

  pk::callback cb1 = pk::make_callback(&f, &functor::member_func);
  pk::callback cb2 = &free_func1;

  ASSERT_NE(cb1, cb2);
  ASSERT_NE(cb2, cb1);

  // Also check whether two callbacks encapsulating the same function/
  // functor compare equal.
  pk::callback cb3 = pk::make_callback(&f, &functor::member_func);
  ASSERT_EQ(cb1, cb3);

  pk::callback cb4 = &free_func1;
  ASSERT_EQ(cb2, cb4);

  // It's equally important that a callback constructed from a different
  // instance of the same functor class compares not equal.
  functor f2;
  pk::callback cb5 = pk::make_callback(&f2, &functor::member_func);
  ASSERT_NE(cb1, cb5);
  ASSERT_NE(cb3, cb5);
}


TEST(Callback, empty)
{
  // Empty/un-assigned callbacks should behave sanely
  pk::callback cb;

  ASSERT_EQ(true, cb.empty());
  ASSERT_FALSE(cb);

  ASSERT_THROW(cb(0, pk::error_t(1), pk::handle(), nullptr), pk::exception);

  pk::callback cb2 = &free_func1;
  ASSERT_NE(cb, cb2);
}


TEST(Callback, assignment)
{
  // Ensure that empty callbacks can be assigned later on.
  pk::callback cb;
  ASSERT_FALSE(cb);

  cb = &free_func1;
  ASSERT_TRUE(cb);
  ASSERT_EQ(false, cb.empty());
  ASSERT_EQ(pk::error_t(1), cb(42, pk::error_t(0), pk::handle::make_dummy(0), nullptr));

  functor f;
  cb = pk::make_callback(&f);
  ASSERT_TRUE(cb);
  ASSERT_EQ(false, cb.empty());
  ASSERT_EQ(pk::error_t(4), cb(0xdeadbeef, pk::error_t(0), pk::handle::make_dummy(0), nullptr));
}


TEST(Callback, hash)
{
  std::hash<pk::callback> hasher;

  // Callbacks made from the same free function should have the same hash.
  pk::callback cb1 = &free_func1;
  pk::callback cb2 = &free_func1;
  ASSERT_EQ(hasher(cb1), hasher(cb2));

  // But they can't have the same hash as a callback made from a different
  // free function.
  pk::callback cb3 = &free_func2;
  ASSERT_NE(hasher(cb1), hasher(cb3));
  ASSERT_NE(hasher(cb2), hasher(cb3));

  // The equality constraint also applies to functors.
  functor f1;
  pk::callback cb4 = pk::make_callback(&f1, &functor::member_func);
  pk::callback cb5 = pk::make_callback(&f1, &functor::member_func);
  ASSERT_EQ(hasher(cb4), hasher(cb5));

  // And the same applies to the non-equality
  functor f2;
  pk::callback cb6 = pk::make_callback(&f2, &functor::member_func);
  ASSERT_NE(hasher(cb4), hasher(cb6));
  ASSERT_NE(hasher(cb5), hasher(cb6));
}


TEST(Callback, copy)
{
  // Copy ctor
  pk::callback cb1 = &free_func1;
  pk::callback cb2 = cb1;
  ASSERT_EQ(cb1, cb2);

  // Assign
  pk::callback cb3;
  cb3 = cb1;
  ASSERT_EQ(cb1, cb3);
}
