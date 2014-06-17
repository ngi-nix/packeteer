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

#include <packeteer/net/detail/cidr.h>

#include <cppunit/extensions/HelperMacros.h>

#include <cstring>

#include <sstream>
#include <string>
#include <set>

namespace pnet = packeteer::net;

namespace {

struct test_data
{
  char const *  netspec;
  bool          no_mask;
  sa_family_t   expected_proto;
  ssize_t       expected_mask;
  uint16_t      port;
  uint16_t      expected_port1;
  uint16_t      expected_port2;
} tests[] = {
  // Garbage (except for port)
  { "asddfs",         true,   AF_UNSPEC, -1, 12345, 0, 12345 },
  { "asddfs",         false,  AF_UNSPEC, -1, 12345, 0, 12345 },

  // IPv4 hosts
  { "192.168.0.1",    true,   AF_INET,    0, 12345, 0, 12345 },
  { "192.168.0.1/24", true,   AF_UNSPEC, -1, 12345, 0, 12345 },

  // IPv4 hosts with port
  { "192.168.0.1:22",    false,  AF_INET,    -1, 0, 22, 22 },
  { "192.168.0.1:22",    false,  AF_INET,    -1, 12345, 22, 12345 },
  { "192.168.0.1:xx",    false,  AF_UNSPEC,  -1, 0, 0, 0 },
  { "192.168.0.1:22/24", false,  AF_UNSPEC,  -1, 0, 0, 0 },

  // IPv4 networks
  { "192.168.0.1/33", false,  AF_INET,   -1, 12345, 0, 12345 },
  { "192.168.0.1/32", false,  AF_INET,   32, 12345, 0, 12345 },
  { "192.168.0.1/31", false,  AF_INET,   31, 12345, 0, 12345 },
  { "192.168.0.1/25", false,  AF_INET,   25, 12345, 0, 12345 },
  { "192.168.0.1/24", false,  AF_INET,   24, 12345, 0, 12345 },
  { "192.168.0.1/23", false,  AF_INET,   23, 12345, 0, 12345 },
  { "192.168.0.1/17", false,  AF_INET,   17, 12345, 0, 12345 },
  { "192.168.0.1/16", false,  AF_INET,   16, 12345, 0, 12345 },
  { "192.168.0.1/15", false,  AF_INET,   15, 12345, 0, 12345 },
  { "192.168.0.1/8",  false,  AF_INET,    8, 12345, 0, 12345 },
  { "192.168.0.1/7",  false,  AF_INET,    7, 12345, 0, 12345 },
  { "192.168.0.1/0",  false,  AF_INET,   -1, 12345, 0, 12345 },

  // IPv6 hosts
  { "2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true,   AF_INET6,   0, 12345, 0, 12345 },
  { "2001:0db8:85a3:0:0:8a2e:0370:7334",          true,   AF_INET6,   0, 12345, 0, 12345 },
  { "2001:0db8:85a3::8a2e:0370:7334",             true,   AF_INET6,   0, 12345, 0, 12345 },
  { "2001:0db8:85a3:0000:0000:8a2e:0370:7334/10", true,   AF_UNSPEC, -1, 12345, 0, 12345 },
  { "2001:0db8:85a3:0:0:8a2e:0370:7334/10",       true,   AF_UNSPEC, -1, 12345, 0, 12345 },
  { "2001:0db8:85a3::8a2e:0370:7334/10",          true,   AF_UNSPEC, -1, 12345, 0, 12345 },

  // IPv6 hosts with port
  { "[2001:0db8:85a3::8a2e:0370:7334]:22",        false,   AF_INET6,  -1, 12345, 22, 12345 },
  { "[2001:0db8:85a3::8a2e:0370:7334]:22",        false,   AF_INET6,  -1, 0, 22, 22 },
  { "[2001:0db8:85a3::8a2e:0370:7334",            false,   AF_UNSPEC, -1, 0, 0, 0 },
  { "[2001:0db8:85a3::8a2e:0370:7334]:ab",        false,   AF_UNSPEC, -1, 0, 0, 0 },
  { "[2001:0db8:85a3::8a2e:0370:7334]:22/24",     false,   AF_UNSPEC, -1, 0, 0, 0 },

  // IPv6 networks
  { "2001:0db8:85a3:0000:0000:8a2e:0370:7334/22", false,  AF_INET6,  22, 12345, 0, 12345 },
  { "2001:0db8:85a3:0:0:8a2e:0370:7334/22",       false,  AF_INET6,  22, 12345, 0, 12345 },
  { "2001:0db8:85a3::8a2e:0370:7334/22",          false,  AF_INET6,  22, 12345, 0, 12345 },

  { "2001:0db8:85a3:0000:0000:8a2e:0370:7334/129",false,  AF_INET6,  -1, 12345, 0, 12345 },
  { "2001:0db8:85a3::8a2e:0370:7334/0",           false,  AF_INET6,  -1, 12345, 0, 12345 },

};

} // anonymous namespace

class CIDRTest
    : public CppUnit::TestFixture
{
public:
  CPPUNIT_TEST_SUITE(CIDRTest);

    CPPUNIT_TEST(testCIDRParsing);

  CPPUNIT_TEST_SUITE_END();

private:

  void testCIDRParsing()
  {
    for (size_t i = 0 ; i < sizeof(tests) / sizeof(test_data) ; ++i) {
      // std::cout << "test: " << tests[i].netspec << std::endl;

      // Parse without specifying port
      pnet::detail::address_type address; 
      sa_family_t proto;
      ssize_t mask = pnet::detail::parse_extended_cidr(tests[i].netspec,
          tests[i].no_mask, address, proto);

      CPPUNIT_ASSERT_EQUAL(tests[i].expected_proto, proto);
      CPPUNIT_ASSERT_EQUAL(tests[i].expected_mask, mask);

      if (AF_INET == proto) {
        sockaddr_in * addr = (sockaddr_in *) &address;
        CPPUNIT_ASSERT_EQUAL(uint16_t(htons(tests[i].expected_port1)), addr->sin_port);
      }
      else if (AF_INET6 == proto) {
        sockaddr_in6 * addr = (sockaddr_in6 *) &address;
        CPPUNIT_ASSERT_EQUAL(uint16_t(htons(tests[i].expected_port1)), addr->sin6_port);
      }

      // Parsing the same thing again with port should change the test
      // results a little.
      ::memset(&address, 0, sizeof(sockaddr_storage));
      mask = pnet::detail::parse_extended_cidr(tests[i].netspec,
          tests[i].no_mask, address, proto, tests[i].port);

      CPPUNIT_ASSERT_EQUAL(tests[i].expected_proto, proto);
      CPPUNIT_ASSERT_EQUAL(tests[i].expected_mask, mask);

      if (AF_INET == proto) {
        sockaddr_in * addr = (sockaddr_in *) &address;
        CPPUNIT_ASSERT_EQUAL(uint16_t(htons(tests[i].expected_port2)), addr->sin_port);
      }
      else if (AF_INET6 == proto) {
        sockaddr_in6 * addr = (sockaddr_in6 *) &address;
        CPPUNIT_ASSERT_EQUAL(uint16_t(htons(tests[i].expected_port2)), addr->sin6_port);
      }

    }
  }


};


CPPUNIT_TEST_SUITE_REGISTRATION(CIDRTest);
