/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@unwesen.co.uk>
 *
 * Copyright (c) 2014 Unwesen Ltd.
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

#include <packeteer/connector.h>

#include <cppunit/extensions/HelperMacros.h>

#include <cstring>

#include <stdexcept>

namespace {

struct test_data
{
  char const *  address;
  bool          valid;
} tests[] = {
  // Garbage
  { "foo", false },
  { "foo:", false },
  { "foo://", false },
  { "foo:///some/path", false },
  { "foo://123.123.133.123:12", false },
  { "tcp://foo", false },
  { "tcp4://foo", false },
  { "tcp6://foo", false },
  { "udp://foo", false },
  { "udp4://foo", false },
  { "udp6://foo", false },
  { "file://", false },
  { "ipc://", false },
  { "pipe://", false },

  // IPv4 hosts
  { "tcp://192.168.0.1",      true },
  { "tcp://192.168.0.1:8080", true },
  { "tCp://192.168.0.1",      true },
  { "tcP://192.168.0.1:8080", true },

  { "tcp4://192.168.0.1",      true },
  { "tcp4://192.168.0.1:8080", true },
  { "tCp4://192.168.0.1",      true },
  { "tcP4://192.168.0.1:8080", true },

  { "tcp4://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    false, },
  { "tcp4://2001:0db8:85a3:0:0:8a2e:0370:7334",          false, },
  { "tcp4://2001:0db8:85a3::8a2e:0370:7334",             false, },
  { "Tcp4://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    false, },
  { "tCp4://2001:0db8:85a3:0:0:8a2e:0370:7334",          false, },
  { "tcP4://2001:0db8:85a3::8a2e:0370:7334",             false, },

  { "udp://192.168.0.1",      true },
  { "udp://192.168.0.1:8080", true },
  { "uDp://192.168.0.1",      true },
  { "udP://192.168.0.1:8080", true },

  { "udp4://192.168.0.1",      true },
  { "udp4://192.168.0.1:8080", true },
  { "uDp4://192.168.0.1",      true },
  { "udP4://192.168.0.1:8080", true },

  { "udp4://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    false, },
  { "udp4://2001:0db8:85a3:0:0:8a2e:0370:7334",          false, },
  { "udp4://2001:0db8:85a3::8a2e:0370:7334",             false, },
  { "Udp4://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    false, },
  { "uDp4://2001:0db8:85a3:0:0:8a2e:0370:7334",          false, },
  { "udP4://2001:0db8:85a3::8a2e:0370:7334",             false, },

  // IPv6 hosts
  { "tcp://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true, },
  { "tcp://2001:0db8:85a3:0:0:8a2e:0370:7334",          true, },
  { "tcp://2001:0db8:85a3::8a2e:0370:7334",             true, },
  { "Tcp://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true, },
  { "tCp://2001:0db8:85a3:0:0:8a2e:0370:7334",          true, },
  { "tcP://2001:0db8:85a3::8a2e:0370:7334",             true, },

  { "tcp6://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true, },
  { "tcp6://2001:0db8:85a3:0:0:8a2e:0370:7334",          true, },
  { "tcp6://2001:0db8:85a3::8a2e:0370:7334",             true, },
  { "Tcp6://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true, },
  { "tCp6://2001:0db8:85a3:0:0:8a2e:0370:7334",          true, },
  { "tcP6://2001:0db8:85a3::8a2e:0370:7334",             true, },

  { "tcp6://192.168.0.1",      false },
  { "tcp6://192.168.0.1:8080", false },
  { "tCp6://192.168.0.1",      false },
  { "tcP6://192.168.0.1:8080", false },

  { "udp://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true, },
  { "udp://2001:0db8:85a3:0:0:8a2e:0370:7334",          true, },
  { "udp://2001:0db8:85a3::8a2e:0370:7334",             true, },
  { "Udp://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true, },
  { "uDp://2001:0db8:85a3:0:0:8a2e:0370:7334",          true, },
  { "udP://2001:0db8:85a3::8a2e:0370:7334",             true, },

  { "udp6://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true, },
  { "udp6://2001:0db8:85a3:0:0:8a2e:0370:7334",          true, },
  { "udp6://2001:0db8:85a3::8a2e:0370:7334",             true, },
  { "Udp6://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true, },
  { "uDp6://2001:0db8:85a3:0:0:8a2e:0370:7334",          true, },
  { "udP6://2001:0db8:85a3::8a2e:0370:7334",             true, },

  { "udp6://192.168.0.1",      false },
  { "udp6://192.168.0.1:8080", false },
  { "udP6://192.168.0.1",      false },
  { "uDp6://192.168.0.1:8080", false },

  // All other types require path names. There's not much common
  // about path names, so our only requirement is that it exists.
  { "file://foo", true },
  { "ipc://foo", true },
  { "pipe://foo", true },

};

} // anonymous namespace

class ConnectorTest
    : public CppUnit::TestFixture
{
public:
  CPPUNIT_TEST_SUITE(ConnectorTest);

    CPPUNIT_TEST(testAddressParsing);

  CPPUNIT_TEST_SUITE_END();

private:

  void testAddressParsing()
  {
    using namespace packeteer;

    for (size_t i = 0 ; i < sizeof(tests) / sizeof(test_data) ; ++i) {
      // std::cout << "test: " << tests[i].address << std::endl;

      if (tests[i].valid) {
        CPPUNIT_ASSERT_NO_THROW(auto c = connector(tests[i].address));
      }
      else {
        CPPUNIT_ASSERT_THROW(auto c = connector(tests[i].address), std::runtime_error);
      }
    }
  }

};


CPPUNIT_TEST_SUITE_REGISTRATION(ConnectorTest);
