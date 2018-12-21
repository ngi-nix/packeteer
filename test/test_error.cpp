/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2017-2019 Jens Finkhaeuser.
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

#include <packeteer/error.h>

#include <errno.h>

#include <string>

#include <cppunit/extensions/HelperMacros.h>

class ErrorTest
    : public CppUnit::TestFixture
{
public:
  CPPUNIT_TEST_SUITE(ErrorTest);

    CPPUNIT_TEST(testBasics);
    CPPUNIT_TEST(testDetails);

  CPPUNIT_TEST_SUITE_END();

private:

  void testBasics()
  {
    try {
      throw packeteer::exception(packeteer::ERR_SUCCESS);
    } catch (packeteer::exception const & ex) {
      CPPUNIT_ASSERT_EQUAL(packeteer::ERR_SUCCESS, ex.code());
      CPPUNIT_ASSERT_EQUAL(std::string("No error"), std::string(ex.what()));
      CPPUNIT_ASSERT_EQUAL(std::string("ERR_SUCCESS"), std::string(ex.name()));
    }
  }


  void testDetails()
  {
    // Without errno
    try {
      throw packeteer::exception(packeteer::ERR_SUCCESS, "foo");
    } catch (packeteer::exception const & ex) {
      CPPUNIT_ASSERT_EQUAL(std::string("foo"), ex.details());
    }

    // With an errno value
    try {
      throw packeteer::exception(packeteer::ERR_SUCCESS, EAGAIN, "foo");
    } catch (packeteer::exception const & ex) {

      std::string prefix = "foo // ";
      CPPUNIT_ASSERT(ex.details().size() > prefix.size());
      CPPUNIT_ASSERT(0 == ex.details().compare(0, prefix.size(), prefix));
    }
  }
};


CPPUNIT_TEST_SUITE_REGISTRATION(ErrorTest);
