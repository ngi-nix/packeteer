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

namespace p7r = packeteer;

namespace {

p7r::error_t
free_func1(p7r::time_point const &, p7r::events_t events,
    p7r::error_t, p7r::connector const &, void *)
{
  EXPECT_EQ(42, events);
  return p7r::error_t(1);
}


p7r::error_t
free_func2(p7r::time_point const &, p7r::events_t events,
    p7r::error_t, p7r::connector const &, void *)
{
  EXPECT_EQ(666, events);
  return p7r::error_t(2);
}


struct functor
{
  p7r::error_t
  member_func(p7r::time_point const &, p7r::events_t events,
      p7r::error_t, p7r::connector const &, void *)
  {
    EXPECT_EQ(1234, events);
    return p7r::error_t(3);
  }



  p7r::error_t
  operator()(p7r::time_point const &, p7r::events_t events,
      p7r::error_t, p7r::connector const &, void *)
  {
    EXPECT_EQ(0xdeadbeef, events);
    return p7r::error_t(4);
  }
};


} // anonymous namespace


TEST(Callback, free_functions)
{
  // Test that a free function is correctly invoked.
  auto now = p7r::clock::now();

  p7r::callback cb1 = &free_func1;
  ASSERT_EQ(p7r::error_t(1), cb1(now, 42, p7r::error_t(0), p7r::connector{},
        nullptr));

  p7r::callback cb2 = &free_func2;
  ASSERT_EQ(p7r::error_t(2), cb2(now, 666, p7r::error_t(0), p7r::connector{},
        nullptr));

  // Test for equality.
  ASSERT_NE(cb1, cb2);
  p7r::callback cb3 = &free_func1;
  ASSERT_EQ(cb1, cb3);
}



TEST(Callback, member_functions)
{
  // Test that member functions are correctly invoked.
  auto now = p7r::clock::now();
  functor f;

  p7r::callback cb1 = p7r::make_callback(&f, &functor::member_func);
  ASSERT_EQ(p7r::error_t(3), cb1(now, 1234, p7r::error_t(0), p7r::connector{},
        nullptr));

  p7r::callback cb2 = p7r::make_callback(&f);
  ASSERT_EQ(p7r::error_t(4), cb2(now, 0xdeadbeef, p7r::error_t(0),
        p7r::connector{}, nullptr));

  // Test for equality.
  ASSERT_NE(cb1, cb2);
  p7r::callback cb3 = p7r::make_callback(&f, &functor::member_func);
  ASSERT_EQ(cb1, cb3);
}



TEST(Callback, comparison)
{
  // Test that a functor and a free function bound to callbacks do not
  // compare equal.
  functor f;

  p7r::callback cb1 = p7r::make_callback(&f, &functor::member_func);
  p7r::callback cb2 = &free_func1;

  ASSERT_NE(cb1, cb2);
  ASSERT_NE(cb2, cb1);

  // Also check whether two callbacks encapsulating the same function/
  // functor compare equal.
  p7r::callback cb3 = p7r::make_callback(&f, &functor::member_func);
  ASSERT_EQ(cb1, cb3);

  p7r::callback cb4 = &free_func1;
  ASSERT_EQ(cb2, cb4);

  // It's equally important that a callback constructed from a different
  // instance of the same functor class compares not equal.
  functor f2;
  p7r::callback cb5 = p7r::make_callback(&f2, &functor::member_func);
  ASSERT_NE(cb1, cb5);
  ASSERT_NE(cb3, cb5);
}


TEST(Callback, empty)
{
  // Empty/un-assigned callbacks should behave sanely
  auto now = p7r::clock::now();
  p7r::callback cb;

  ASSERT_EQ(true, cb.empty());
  ASSERT_FALSE(cb);

  ASSERT_THROW(cb(now, 0, p7r::error_t(1), p7r::connector{}, nullptr),
      p7r::exception);

  p7r::callback cb2 = &free_func1;
  ASSERT_NE(cb, cb2);
}


TEST(Callback, assignment)
{
  // Ensure that empty callbacks can be assigned later on.
  auto now = p7r::clock::now();
  p7r::callback cb;
  ASSERT_FALSE(cb);

  cb = &free_func1;
  ASSERT_TRUE(cb);
  ASSERT_EQ(false, cb.empty());
  ASSERT_EQ(p7r::error_t(1), cb(now, 42, p7r::error_t(0), p7r::connector{},
        nullptr));

  functor f;
  cb = p7r::make_callback(&f);
  ASSERT_TRUE(cb);
  ASSERT_EQ(false, cb.empty());
  ASSERT_EQ(p7r::error_t(4), cb(now, 0xdeadbeef, p7r::error_t(0),
        p7r::connector{}, nullptr));
}


TEST(Callback, hash)
{
  std::hash<p7r::callback> hasher;

  // Callbacks made from the same free function should have the same hash.
  p7r::callback cb1 = &free_func1;
  p7r::callback cb2 = &free_func1;
  ASSERT_EQ(hasher(cb1), hasher(cb2));

  // But they can't have the same hash as a callback made from a different
  // free function.
  p7r::callback cb3 = &free_func2;
  ASSERT_NE(hasher(cb1), hasher(cb3));
  ASSERT_NE(hasher(cb2), hasher(cb3));

  // The equality constraint also applies to functors.
  functor f1;
  p7r::callback cb4 = p7r::make_callback(&f1, &functor::member_func);
  p7r::callback cb5 = p7r::make_callback(&f1, &functor::member_func);
  ASSERT_EQ(hasher(cb4), hasher(cb5));

  // And the same applies to the non-equality
  functor f2;
  p7r::callback cb6 = p7r::make_callback(&f2, &functor::member_func);
  ASSERT_NE(hasher(cb4), hasher(cb6));
  ASSERT_NE(hasher(cb5), hasher(cb6));
}


TEST(Callback, copy)
{
  // Copy ctor
  p7r::callback cb1 = &free_func1;
  p7r::callback cb2 = cb1;
  ASSERT_EQ(cb1, cb2);

  // Assign
  p7r::callback cb3;
  cb3 = cb1;
  ASSERT_EQ(cb1, cb3);
}
