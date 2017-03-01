/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2017 Jens Finkhaeuser.
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
#include <packeteer/handle.h>

#include <cppunit/extensions/HelperMacros.h>

namespace pk = packeteer;


class HandleTest
    : public CppUnit::TestFixture
{
public:
  CPPUNIT_TEST_SUITE(HandleTest);

    CPPUNIT_TEST(testBasicFunctionality);

  CPPUNIT_TEST_SUITE_END();

private:

  void testBasicFunctionality()
  {
    // Default-constructed handles are invalid
    pk::handle h1;
    CPPUNIT_ASSERT(!h1.valid());

    // Two default-constructed handles should have the same hash value.
    pk::handle h2;
    CPPUNIT_ASSERT_EQUAL(h1.hash(), h2.hash());

    // In fact, they should be equal
    CPPUNIT_ASSERT(h1 == h2);
    CPPUNIT_ASSERT(!(h1 < h2));
    CPPUNIT_ASSERT(!(h2 < h1));

    // Copying should not change this at all
    pk::handle copy = h2;
    CPPUNIT_ASSERT(h1 == copy);
    CPPUNIT_ASSERT(!(h1 < copy));
    CPPUNIT_ASSERT(!(copy < h1));
  }
};


CPPUNIT_TEST_SUITE_REGISTRATION(HandleTest);
