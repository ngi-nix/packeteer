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
#include <packetflinger/pipe.h>

#include <cppunit/extensions/HelperMacros.h>

namespace pf = packetflinger;


class PipeTest
    : public CppUnit::TestFixture
{
public:
  CPPUNIT_TEST_SUITE(PipeTest);

    CPPUNIT_TEST(testBasicFunctionality);

  CPPUNIT_TEST_SUITE_END();

private:

  void testBasicFunctionality()
  {
    // Test writing to and reading from a pipe works as expected.
    pf::pipe pipe;

    std::string msg = "hello, world!";
    CPPUNIT_ASSERT_EQUAL(pf::ERR_SUCCESS, pipe.write(msg.c_str(), msg.size()));

    std::vector<char> result;
    result.reserve(2 * msg.size());
    size_t amount = 0;
    CPPUNIT_ASSERT_EQUAL(pf::ERR_SUCCESS, pipe.read(&result[0], result.capacity(),
          amount));
    CPPUNIT_ASSERT_EQUAL(msg.size(), amount);

    for (int i = 0 ; i < msg.size() ; ++i) {
      CPPUNIT_ASSERT_EQUAL(msg[i], result[i]);
    }
  }
};


CPPUNIT_TEST_SUITE_REGISTRATION(PipeTest);
