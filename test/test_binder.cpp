/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2014 Unwesen Ltd.
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

#include <gtest/gtest.h>

#include <packeteer/thread/binder.h>

namespace {

struct member
{
  bool called = false;

  void mem1()
  {
    called = true;
  }

  void mem2(int)
  {
    called = true;
  }

  int mem3()
  {
    called = true;
    return 42;
  }

  void mem4() const
  {
    const_cast<member *>(this)->called = true;
  }
};


} // anonymous namespace

TEST(Binder, void_member_by_pointer)
{
  member m;
  EXPECT_FALSE(m.called);

  auto f = packeteer::thread::binder(&m, &member::mem1);
  f();
  ASSERT_TRUE(m.called);
}

TEST(Binder, int_member_by_pointer)
{
  member m;
  EXPECT_FALSE(m.called);

  auto f = packeteer::thread::binder(&m, &member::mem2);
  f(42);
  ASSERT_TRUE(m.called);
}

TEST(Binder, int_returning_member_by_pointer)
{
  member m;
  EXPECT_FALSE(m.called);

  auto f = packeteer::thread::binder(&m, &member::mem3);
  EXPECT_FALSE(m.called);
  int x = f();
  ASSERT_TRUE(m.called);
  ASSERT_EQ(42, x);
}

TEST(Binder, const_member_by_pointer)
{
  member m;
  EXPECT_FALSE(m.called);
  member const * m2 = &m;

  auto f = packeteer::thread::binder(m2, &member::mem4);
  EXPECT_FALSE(m.called);
  f();
  ASSERT_TRUE(m.called);
}

TEST(Binder, void_member_by_reference)
{
  member m;
  EXPECT_FALSE(m.called);

  auto f = packeteer::thread::binder(m, &member::mem1);
  f();
  ASSERT_TRUE(m.called);
}

TEST(Binder, int_member_by_reference)
{
  member m;
  EXPECT_FALSE(m.called);

  auto f = packeteer::thread::binder(m, &member::mem2);
  f(42);
  ASSERT_TRUE(m.called);
}

TEST(Binder, int_returning_member_by_reference)
{
  member m;
  EXPECT_FALSE(m.called);

  auto f = packeteer::thread::binder(m, &member::mem3);
  EXPECT_FALSE(m.called);
  int x = f();
  ASSERT_TRUE(m.called);
  ASSERT_EQ(42, x);
}

TEST(Binder, const_member_by_reference)
{
  member m;
  EXPECT_FALSE(m.called);
  member const & m2 = m;

  auto f = packeteer::thread::binder(m2, &member::mem4);
  EXPECT_FALSE(m.called);
  f();
  ASSERT_TRUE(m.called);
}
