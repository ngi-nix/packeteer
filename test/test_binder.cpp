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

#include <cppunit/extensions/HelperMacros.h>

#include <packeteer/thread/binder.h>

namespace {

bool called = false;

struct member
{
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
    called = true;
  }
};


} // anonymous namespace

class BinderTest
    : public CppUnit::TestFixture
{
public:
    CPPUNIT_TEST_SUITE(BinderTest);

      CPPUNIT_TEST(testMemberByPointer);
      CPPUNIT_TEST(testMemberByReference);

    CPPUNIT_TEST_SUITE_END();

private:

    void testMemberByPointer()
    {
      member m;

      called = false;
      std::function<void()> f1 = packeteer::thread::binder(&m, &member::mem1);
      CPPUNIT_ASSERT(!called);
      f1();
      CPPUNIT_ASSERT(called);

      called = false;
      std::function<void(int)> f2 = packeteer::thread::binder(&m, &member::mem2);
      CPPUNIT_ASSERT(!called);
      f2(42);
      CPPUNIT_ASSERT(called);

      called = false;
      std::function<int()> f3 = packeteer::thread::binder(&m, &member::mem3);
      CPPUNIT_ASSERT(!called);
      int x = f3();
      CPPUNIT_ASSERT(called);
      CPPUNIT_ASSERT_EQUAL(42, x);


      member const * m2 = &m;
      called = false;
      std::function<void()> f4 = packeteer::thread::binder(m2, &member::mem4);
      CPPUNIT_ASSERT(!called);
      f4();
      CPPUNIT_ASSERT(called);
    }



    void testMemberByReference()
    {
      member m;

      called = false;
      std::function<void()> f1 = packeteer::thread::binder(m, &member::mem1);
      CPPUNIT_ASSERT(!called);
      f1();
      CPPUNIT_ASSERT(called);

      called = false;
      std::function<void(int)> f2 = packeteer::thread::binder(m, &member::mem2);
      CPPUNIT_ASSERT(!called);
      f2(42);
      CPPUNIT_ASSERT(called);

      called = false;
      std::function<int()> f3 = packeteer::thread::binder(m, &member::mem3);
      CPPUNIT_ASSERT(!called);
      int x = f3();
      CPPUNIT_ASSERT(called);
      CPPUNIT_ASSERT_EQUAL(42, x);


      member const & m2 = m;
      called = false;
      std::function<void()> f4 = packeteer::thread::binder(m2, &member::mem4);
      CPPUNIT_ASSERT(!called);
      f4();
      CPPUNIT_ASSERT(called);
    }
};


CPPUNIT_TEST_SUITE_REGISTRATION(BinderTest);
