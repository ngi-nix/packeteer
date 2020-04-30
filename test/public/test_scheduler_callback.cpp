/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 * Copyright (c) 2012-2014 Unwesen Ltd.
 * Copyright (c) 2015-2020 Jens Finkhaeuser.
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
    p7r::error_t, p7r::connector *, void *)
{
  EXPECT_EQ(42, events);
  return p7r::error_t(1);
}


p7r::error_t
free_func2(p7r::time_point const &, p7r::events_t events,
    p7r::error_t, p7r::connector *, void *)
{
  EXPECT_EQ(666, events);
  return p7r::error_t(2);
}



struct functor_member
{
  p7r::error_t
  member_func(p7r::time_point const &, p7r::events_t events,
      p7r::error_t, p7r::connector *, void *)
  {
    EXPECT_EQ(1234, events);
    return p7r::error_t(3);
  }
};



struct true_functor
{
  p7r::error_t
  operator()(p7r::time_point const &, p7r::events_t events,
      p7r::error_t, p7r::connector *, void *)
  {
    EXPECT_EQ(0xdeadbeef, events);
    return p7r::error_t(4);
  }
};


inline p7r::callback
make_test_cb()
{
  std::string test = "Test";
  auto lambda = [test](p7r::time_point const &, p7r::events_t,
    p7r::error_t, p7r::connector *, void *) -> p7r::error_t
  {
    EXPECT_EQ("Test", test);
    return 2;
  };

  return lambda;
}


} // anonymous namespace



TEST(Callback, free_functions)
{
  // Test that a free function is correctly invoked.
  auto now = p7r::clock::now();

  p7r::callback cb1 = &free_func1;
  ASSERT_EQ(p7r::error_t(1), cb1(now, 42, p7r::error_t(0), nullptr,
        nullptr));

  p7r::callback cb2 = &free_func2;
  ASSERT_EQ(p7r::error_t(2), cb2(now, 666, p7r::error_t(0), nullptr,
        nullptr));

  // Test for equality.
  ASSERT_NE(cb1, cb2);
  p7r::callback cb3 = &free_func1;
  ASSERT_EQ(cb1, cb3);
}



TEST(Callback, lambda_without_capture)
{
  // Test that a lambda function is correctly invoked.
  auto now = p7r::clock::now();

  auto l1 = [](p7r::time_point const &, p7r::events_t,
    p7r::error_t, p7r::connector *, void *) -> p7r::error_t
  {
    return 1;
  };

  p7r::callback cb1{l1};
  ASSERT_EQ(p7r::error_t{1}, cb1(now, 42, p7r::error_t(0), nullptr,
        nullptr));

  // Equality tests
  p7r::callback cb2{l1};
  ASSERT_EQ(cb1, cb2);

  // An identical lambda should not be equal
  auto l2 = [](p7r::time_point const &, p7r::events_t,
    p7r::error_t, p7r::connector *, void *) -> p7r::error_t
  {
    return 1;
  };

  p7r::callback cb3{l2};
  ASSERT_NE(cb1, cb3);
  ASSERT_NE(cb2, cb3);
}



TEST(Callback, lambda_with_capture)
{
  // Lambda with capture
  int dummy = 42;
  auto l1 = [&dummy](p7r::time_point const &, p7r::events_t,
    p7r::error_t, p7r::connector *, void *) -> p7r::error_t
  {
    return 1;
  };

  // Test that a lambda function is correctly invoked.
  auto now = p7r::clock::now();

  p7r::callback cb1{l1};
  ASSERT_EQ(p7r::error_t{1}, cb1(now, 42, p7r::error_t(0), nullptr,
        nullptr));

  // Equality tests
  p7r::callback cb2{l1};
  ASSERT_EQ(cb1, cb2);

  // An identical lambda should not be equal
  auto l2 = [&dummy](p7r::time_point const &, p7r::events_t,
    p7r::error_t, p7r::connector *, void *) -> p7r::error_t
  {
    return 1;
  };

  p7r::callback cb3{l2};
  ASSERT_NE(cb1, cb3);
  ASSERT_NE(cb2, cb3);

  // Test a lambda with a string capture actually has the string. The captured
  // variable goes out of scope, but it's capture-by-value. Then to complicate
  // things, we assing the callback further.
  p7r::callback cb4 = make_test_cb();
  p7r::callback cb5 = cb4;

  ASSERT_EQ(p7r::error_t{2}, cb5(now, 42, p7r::error_t(0), nullptr,
        nullptr));
}



TEST(Callback, member_functions_by_address)
{
  // Test that member functions are correctly invoked.
  auto now = p7r::clock::now();
  functor_member f;

  p7r::callback cb1{&f, &functor_member::member_func};
  ASSERT_EQ(p7r::error_t(3), cb1(now, 1234, p7r::error_t(0), nullptr,
        nullptr));

  // Test for equality.
  p7r::callback cb2{&f, &functor_member::member_func};
  ASSERT_EQ(cb1, cb2);
}



TEST(Callback, member_functions_copy)
{
  // Test that member functions are correctly invoked.
  auto now = p7r::clock::now();
  functor_member f;

  p7r::callback cb1{f, &functor_member::member_func};
  ASSERT_EQ(p7r::error_t(3), cb1(now, 1234, p7r::error_t(0), nullptr,
        nullptr));

  // Test for equality.
  p7r::callback cb2{f, &functor_member::member_func};
  ASSERT_EQ(cb1, cb2);
}




TEST(Callback, true_functor_by_address)
{
  // Test that member functions are correctly invoked.
  auto now = p7r::clock::now();
  true_functor f;

  p7r::callback cb1{&f};
  ASSERT_EQ(p7r::error_t(4), cb1(now, 0xdeadbeef, p7r::error_t(0), nullptr,
        nullptr));

  // Test for equality.
  p7r::callback cb2{&f};
  ASSERT_EQ(cb1, cb2);
}



TEST(Callback, true_functor_copy)
{
  // Test that member functions are correctly invoked.
  auto now = p7r::clock::now();
  true_functor f;

  p7r::callback cb1{f};
  ASSERT_EQ(p7r::error_t(4), cb1(now, 0xdeadbeef, p7r::error_t(0),
        nullptr, nullptr));

  // Test for equality.
  p7r::callback cb2{f};
  ASSERT_EQ(cb1, cb2);
}



TEST(Callback, comparison)
{
  // Test that a functor and a free function bound to callbacks do not
  // compare equal.
  functor_member f;

  p7r::callback cb1{&f, &functor_member::member_func};
  p7r::callback cb2 = &free_func1;

  ASSERT_NE(cb1, cb2);
  ASSERT_NE(cb2, cb1);

  // Also check whether two callbacks encapsulating the same function/
  // functor compare equal.
  p7r::callback cb3{&f, &functor_member::member_func};
  ASSERT_EQ(cb1, cb3);

  p7r::callback cb4 = &free_func1;
  ASSERT_EQ(cb2, cb4);

  // It's equally important that a callback constructed from a different
  // instance of the same functor class compares not equal.
  functor_member f2;
  p7r::callback cb5{&f2, &functor_member::member_func};
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

  ASSERT_THROW(cb(now, 0, p7r::error_t(1), nullptr, nullptr),
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
  ASSERT_EQ(p7r::error_t(1), cb(now, 42, p7r::error_t(0), nullptr,
        nullptr));

  true_functor f;
  cb = &f;
  ASSERT_TRUE(cb);
  ASSERT_EQ(false, cb.empty());
  ASSERT_EQ(p7r::error_t(4), cb(now, 0xdeadbeef, p7r::error_t(0),
        nullptr, nullptr));
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
  functor_member f1;
  p7r::callback cb4{&f1, &functor_member::member_func};
  p7r::callback cb5{&f1, &functor_member::member_func};
  ASSERT_EQ(hasher(cb4), hasher(cb5));

  // And the same applies to the non-equality
  functor_member f2;
  p7r::callback cb6{&f2, &functor_member::member_func};
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
