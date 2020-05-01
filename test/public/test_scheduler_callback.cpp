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

#include "../value_tests.h"
#include "../test_name.h"


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






struct test_data_base
{
  std::string     name;
  p7r::events_t   events;
  p7r::error_t    result;

  p7r::callback   cb1 = {};
  p7r::callback   cb2 = {};

  test_data_base(std::string const & n, p7r::events_t ev,
      p7r::error_t res)
    : name(n)
    , events(ev)
    , result(res)
  {
  }

  virtual ~test_data_base() {};
};


struct free_function_ctx : public test_data_base
{
  free_function_ctx(std::string const & n, p7r::events_t ev,
      p7r::error_t res)
    : test_data_base(n, ev, res)
  {
    cb1 = free_func1;
    cb2 = free_func2;
  }
};


struct functor_member_ctx : public test_data_base
{
  functor_member fm1;
  functor_member fm2;

  functor_member_ctx(std::string const & n, p7r::events_t ev,
      p7r::error_t res, bool by_pointer)
    : test_data_base(n, ev, res)
  {
    if (by_pointer) {
      cb1 = {&fm1, &functor_member::member_func};
      cb2 = {&fm2, &functor_member::member_func};
    }
    else {
      cb1 = {fm1, &functor_member::member_func};
      cb2 = {fm2, &functor_member::member_func};
    }
  }
};


struct true_functor_ctx : public test_data_base
{
  true_functor tf1;
  true_functor tf2;

  true_functor_ctx(std::string const & n, p7r::events_t ev,
      p7r::error_t res, bool by_pointer)
    : test_data_base(n, ev, res)
  {
    if (by_pointer) {
      cb1 = {&tf1};
      cb2 = {&tf2};
    }
    else {
      cb1 = {tf1};
      cb2 = {tf2};
    }
  }
};


struct lambda_no_capture_ctx : public test_data_base
{
  lambda_no_capture_ctx(std::string const & n, p7r::events_t ev,
      p7r::error_t res)
    : test_data_base(n, ev, res)
  {
    cb1 = [](p7r::time_point const &, p7r::events_t events, p7r::error_t,
        p7r::connector *, void *) -> p7r::error_t
    {
      EXPECT_EQ(71, events);
      return 5;
    };

    // Same definition, but different lambda
    cb2 = [](p7r::time_point const &, p7r::events_t events, p7r::error_t,
        p7r::connector *, void *) -> p7r::error_t
    {
      EXPECT_EQ(71, events);
      return 5;
    };
  }
};


struct lambda_with_capture_ctx : public test_data_base
{
  int the_capture = 42;

  lambda_with_capture_ctx(std::string const & n, p7r::events_t ev,
      p7r::error_t res, bool by_reference)
    : test_data_base(n, ev, res)
  {
    int & capture = the_capture;

    if (by_reference) {
      cb1 = [&capture](p7r::time_point const &, p7r::events_t events, p7r::error_t,
          p7r::connector *, void *) -> p7r::error_t
      {
        EXPECT_EQ(73, events);
        EXPECT_EQ(42, capture);
        return 6;
      };

      // Same definition, but different lambda
      cb2 = [&capture](p7r::time_point const &, p7r::events_t events, p7r::error_t,
          p7r::connector *, void *) -> p7r::error_t
      {
        EXPECT_EQ(73, events);
        EXPECT_EQ(42, capture);
        return 6;
      };
    }
    else {
      cb1 = [capture](p7r::time_point const &, p7r::events_t events, p7r::error_t,
          p7r::connector *, void *) -> p7r::error_t
      {
        EXPECT_EQ(79, events);
        EXPECT_EQ(42, capture);
        return 7;
      };

      // Same definition, but different lambda
      cb2 = [capture](p7r::time_point const &, p7r::events_t events, p7r::error_t,
          p7r::connector *, void *) -> p7r::error_t
      {
        EXPECT_EQ(79, events);
        EXPECT_EQ(42, capture);
        return 7;
      };
    }
  }
};



std::shared_ptr<test_data_base> test_data[] = {
  std::make_shared<free_function_ctx>(
      "free function", 42, 1
  ),
  std::make_shared<functor_member_ctx>(
      "functor member pointer", 1234, 3,
      true
  ),
  std::make_shared<functor_member_ctx>(
      "functor member copy", 1234, 3,
      false
  ),
  std::make_shared<true_functor_ctx>(
      "true functor pointer", 0xdeadbeef, 4,
      true
  ),
  std::make_shared<true_functor_ctx>(
      "true functor copy", 0xdeadbeef, 4,
      false
  ),
  std::make_shared<lambda_no_capture_ctx>(
    "lambda without capture", 71, 5
  ),
  std::make_shared<lambda_with_capture_ctx>(
    "lambda with reference capture", 73, 6,
    true
  ),
  std::make_shared<lambda_with_capture_ctx>(
    "lambda with copy capture", 79, 7,
    false
  ),
};



std::string generate_name(testing::TestParamInfo<std::shared_ptr<test_data_base>> const & info)
{
  return symbolize_name(info.param->name);
}

} // anonymoys namespace

class Callback
  : public testing::TestWithParam<std::shared_ptr<test_data_base>>
{
};



TEST_P(Callback, copy_construct)
{
  auto td = GetParam();
  auto original = td->cb1;

  // Copy ctor
  p7r::callback cb1 = original;
  ASSERT_EQ(cb1, original);
}



TEST_P(Callback, assign)
{
  auto td = GetParam();
  auto original = td->cb1;

  // Assign
  p7r::callback cb1;
  ASSERT_NE(cb1, original);

  cb1 = original;
  ASSERT_EQ(cb1, original);
}



TEST_P(Callback, compare_inequality)
{
  auto td = GetParam();
  ASSERT_NE(td->cb1, td->cb2);
}



TEST_P(Callback, invoke)
{
  auto td = GetParam();
  auto cb = td->cb1;

  auto now = p7r::clock::now();
  auto res = cb(now, td->events, p7r::error_t(0), nullptr, nullptr);
  ASSERT_EQ(td->result, res);
}


TEST_P(Callback, hash)
{
  auto td = GetParam();

  std::hash<p7r::callback> hasher;

  auto copy = td->cb1;

  ASSERT_EQ(hasher(td->cb1), hasher(copy));
  ASSERT_NE(hasher(td->cb1), hasher(td->cb2));
}


INSTANTIATE_TEST_SUITE_P(scheduler, Callback,
    testing::ValuesIn(test_data),
    generate_name);



TEST(CallbackMisc, empty)
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




