/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2014 Unwesen Ltd.
 * Copyright (c) 2015-2017 Jens Finkhaeuser.
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

#include <packeteer/net/socket_address.h>

#include <cppunit/extensions/HelperMacros.h>

#include <cstring>

#include <sstream>
#include <string>
#include <set>

#include "value_tests.h"

namespace pnet = packeteer::net;

namespace {

struct test_data
{
  int                                       type;
  pnet::socket_address::socket_address_type sa_type;
  char const *                              address;
  char const *                              expected;
  uint16_t                                  port;
} tests[] = {
  { AF_INET,  pnet::socket_address::SAT_INET4, "192.168.0.1", "192.168.0.1", 12344, },
  { AF_INET,  pnet::socket_address::SAT_INET4, "192.168.0.1", "192.168.0.1", 12345, },
  { AF_INET6, pnet::socket_address::SAT_INET6, "2001:0db8:85a3:0000:0000:8a2e:0370:7334",
    "2001:db8:85a3::8a2e:370:7334", 12345, },
  { AF_INET6, pnet::socket_address::SAT_INET6, "2001:db8:85a3:0:0:8a2e:370:7334",
    "2001:db8:85a3::8a2e:370:7334", 12345, },
  { AF_INET6, pnet::socket_address::SAT_INET6, "2001:db8:85a3::8a2e:370:7334",
    "2001:db8:85a3::8a2e:370:7334", 12344, },
  { AF_INET6, pnet::socket_address::SAT_INET6, "2001:db8:85a3::8a2e:370:7334",
    "2001:db8:85a3::8a2e:370:7334", 12345, },
  { AF_INET6, pnet::socket_address::SAT_INET6, "0:0:0:0:0:0:0:1", "::1", 12345, },
  { AF_INET6, pnet::socket_address::SAT_INET6, "::1", "::1", 12345, },
  { AF_INET6, pnet::socket_address::SAT_INET6, "0:0:0:0:0:0:0:0", "::", 12345, },
  { AF_INET6, pnet::socket_address::SAT_INET6, "::", "::", 12345, },
  { AF_LOCAL, pnet::socket_address::SAT_LOCAL, "/foo/bar", "/foo/bar", 0 },
  { AF_LOCAL, pnet::socket_address::SAT_LOCAL, "something else", "something else", 0 },
};


inline std::string
full_expected(test_data const & td, uint16_t port = 0)
{
  std::stringstream s;

  if (pnet::socket_address::SAT_INET6 == td.sa_type) {
    s << "[";
  }

  s << td.expected;

  if (pnet::socket_address::SAT_INET6 == td.sa_type) {
    s << "]";
  }

  if (pnet::socket_address::SAT_LOCAL != td.sa_type) {
    s << ":" << port;
  }

  return s.str();
}



} // anonymous namespace

class SocketAddressTest
    : public CppUnit::TestFixture
{
public:
  CPPUNIT_TEST_SUITE(SocketAddressTest);

    CPPUNIT_TEST(testVerifyCIDR);
    CPPUNIT_TEST(testRawConstruction);
    CPPUNIT_TEST(testStringConstruction);
    CPPUNIT_TEST(testHash);
    CPPUNIT_TEST(testOperators);

  CPPUNIT_TEST_SUITE_END();

private:

  pnet::socket_address create_address(test_data const & data)
  {
    using namespace pnet;

    // IPv4
    if (AF_INET == data.type) {
      sockaddr_in addr;
      addr.sin_family = AF_INET;
      addr.sin_port = htons(data.port);
      inet_pton(AF_INET, data.address, &addr.sin_addr);

      return socket_address(&addr, sizeof(addr));
    }

    // IPv6
    if (AF_INET6 == data.type) {
      sockaddr_in6 addr;
      addr.sin6_family = AF_INET6;
      addr.sin6_port = htons(data.port);
      inet_pton(AF_INET6, data.address, &addr.sin6_addr);

      return socket_address(&addr, sizeof(addr));
    }

    // UNIX
    sockaddr_un addr;
    addr.sun_family = AF_LOCAL;
    ::snprintf(addr.sun_path, UNIX_PATH_MAX, "%s", data.address);

    return socket_address(&addr, sizeof(addr));
  }



  void testVerifyCIDR()
  {
    // Tests that the verify_cidr() function works as expected.
    using namespace pnet;

    for (size_t i = 0 ; i < sizeof(tests) / sizeof(test_data) ; ++i) {
      if (socket_address::SAT_LOCAL == tests[i].sa_type) {
        continue;
      }
      CPPUNIT_ASSERT(socket_address::verify_cidr(tests[i].address));
    }
  }



  void testRawConstruction()
  {
    // Tests that information doesn't get mangled during construction or
    // formatting
    using namespace pnet;

    for (size_t i = 0 ; i < sizeof(tests) / sizeof(test_data) ; ++i) {
      socket_address address = create_address(tests[i]);

      CPPUNIT_ASSERT_EQUAL(tests[i].sa_type, address.type());
      if (socket_address::SAT_LOCAL != tests[i].sa_type) {
        CPPUNIT_ASSERT_EQUAL(std::string(tests[i].expected), address.cidr_str());
      }
      CPPUNIT_ASSERT_EQUAL(tests[i].port, address.port());

      std::stringstream s;
      s << address;

      std::string s2 = full_expected(tests[i], tests[i].port);

      CPPUNIT_ASSERT_EQUAL(s2, s.str());
    }
  }



  void testStringConstruction()
  {
    // Tests that information doesn't get mangled during construction or
    // formatting
    using namespace pnet;

    for (size_t i = 0 ; i < sizeof(tests) / sizeof(test_data) ; ++i) {
      // *** Without port in ctor
      {
        // std::cout << "=== " << tests[i].address << " without port" << std::endl;
        socket_address address(tests[i].address);

        CPPUNIT_ASSERT_EQUAL(tests[i].sa_type, address.type());
        if (socket_address::SAT_LOCAL != tests[i].sa_type) {
         CPPUNIT_ASSERT_EQUAL(std::string(tests[i].expected), address.cidr_str());
        }
        CPPUNIT_ASSERT_EQUAL(uint16_t(0), address.port()); // No port in ctor

        std::stringstream s;
        s << address;

        std::string s2 = full_expected(tests[i], 0); // No port in ctor

        CPPUNIT_ASSERT_EQUAL(s2, s.str());

        // Let's also test the verify_netmask() function.
        if (socket_address::SAT_LOCAL != tests[i].sa_type) {
          size_t max = AF_INET == tests[i].type ? 32 : 128;
          for (size_t j = 0 ; j <= max ; ++j) {
            CPPUNIT_ASSERT(address.verify_netmask(j));
          }

          // +1 can't work.
          ++max;
          CPPUNIT_ASSERT(!address.verify_netmask(max));
        }
      }

      // *** With port in ctor
      {
        // std::cout << "=== " << tests[i].address << " with port " << tests[i].port << std::endl;
        socket_address address(tests[i].address, tests[i].port);

        CPPUNIT_ASSERT_EQUAL(tests[i].sa_type, address.type());
        if (socket_address::SAT_LOCAL != tests[i].sa_type) {
          CPPUNIT_ASSERT_EQUAL(std::string(tests[i].expected), address.cidr_str());
        }
        CPPUNIT_ASSERT_EQUAL(tests[i].port, address.port());

        std::stringstream s;
        s << address;

        std::string s2 = full_expected(tests[i], tests[i].port);

        CPPUNIT_ASSERT_EQUAL(s2, s.str());

        // Let's also test the verify_netmask() function.
        if (socket_address::SAT_LOCAL != tests[i].sa_type) {
          size_t max = AF_INET == tests[i].type ? 32 : 128;
          for (size_t j = 0 ; j <= max ; ++j) {
            CPPUNIT_ASSERT(address.verify_netmask(j));
          }

          // +1 can't work.
          ++max;
          CPPUNIT_ASSERT(!address.verify_netmask(max));
        }
      }

    }
  }



  void testHash()
  {
    // Tests that all unique IP addresses in the tests generate unique hashes.
    using namespace pnet;

    std::set<size_t> hashes;
    std::hash<socket_address> hasher;

    for (size_t i = 0 ; i < sizeof(tests) / sizeof(test_data) ; ++i) {
      socket_address address = create_address(tests[i]);
      hashes.insert(hasher(address));
    }

    // We have only 8 unique addresses - two where only the port differs.
    CPPUNIT_ASSERT_EQUAL(size_t(8), hashes.size());
  }



  void testOperators()
  {
    using namespace pnet;

    // *** IPv4
    PACKETEER_VALUES_TEST(socket_address("192.168.0.1"),
                          socket_address("192.168.0.2"),
                          false);
    // Increment
    socket_address s4("192.168.0.1");
    CPPUNIT_ASSERT_EQUAL(socket_address("192.168.0.1"), s4);
    ++s4;
    CPPUNIT_ASSERT_EQUAL(socket_address("192.168.0.2"), s4);


    // *** IPv6
    PACKETEER_VALUES_TEST(socket_address("2001:0db8:85a3::8a2e:0370:7334"),
                          socket_address("2001:0db8:85a3::8a2e:0370:7335"),
                          false);
    // Increment
    socket_address s6("2001:0db8:85a3::8a2e:0370:7334");
    CPPUNIT_ASSERT_EQUAL(socket_address("2001:0db8:85a3::8a2e:0370:7334"), s6);
    ++s6;
    CPPUNIT_ASSERT_EQUAL(socket_address("2001:0db8:85a3::8a2e:0370:7335"), s6);

    // *** IPv4 with port
    PACKETEER_VALUES_TEST(socket_address("192.168.0.1", 1234),
                          socket_address("192.168.0.1", 4321),
                          false);

    // *** IPv6 with port
    PACKETEER_VALUES_TEST(socket_address("2001:0db8:85a3::8a2e:0370:7334", 1234),
                          socket_address("2001:0db8:85a3::8a2e:0370:7334", 4321),
                          false);

    // *** Unix paths
    PACKETEER_VALUES_TEST(socket_address("/foo/bar"),
                          socket_address("/foo/baz"),
                          false);
  }
};


CPPUNIT_TEST_SUITE_REGISTRATION(SocketAddressTest);
