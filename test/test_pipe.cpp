/**
 * This file is part of packetflinger.
 *
 * Author(s): Jens Finkhaeuser <unwesen@users.sourceforge.net>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
