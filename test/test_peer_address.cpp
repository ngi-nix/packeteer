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

#include <packeteer/peer_address.h>

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
  std::string                               scheme;
  std::string                               address;
  packeteer::connector_type                 type;
  pnet::socket_address::socket_address_type sa_type;
  std::string                               expected;
} tests[] = {
  // All schemes, simple.
  { "tcp4",  "tcp4://192.168.0.1", packeteer::CT_TCP4,  pnet::socket_address::SAT_INET4,  "tcp4://192.168.0.1:0", },
  { "tcp4",  "tcp://192.168.0.1",  packeteer::CT_TCP4,  pnet::socket_address::SAT_INET4,  "tcp4://192.168.0.1:0", },
  { "tcp6",  "tcp6://::1",         packeteer::CT_TCP6,  pnet::socket_address::SAT_INET6,  "tcp6://[::1]:0",       },
  { "tcp6",  "tcp://::1",          packeteer::CT_TCP6,  pnet::socket_address::SAT_INET6,  "tcp6://[::1]:0",       },
  { "udp4",  "udp4://192.168.0.1", packeteer::CT_UDP4,  pnet::socket_address::SAT_INET4,  "udp4://192.168.0.1:0", },
  { "udp4",  "udp://192.168.0.1",  packeteer::CT_UDP4,  pnet::socket_address::SAT_INET4,  "udp4://192.168.0.1:0", },
  { "udp6",  "udp6://::1",         packeteer::CT_UDP6,  pnet::socket_address::SAT_INET6,  "udp6://[::1]:0",       },
  { "udp6",  "udp://::1",          packeteer::CT_UDP6,  pnet::socket_address::SAT_INET6,  "udp6://[::1]:0",       },
  { "anon",  "anon://",            packeteer::CT_ANON,  pnet::socket_address::SAT_UNSPEC, "anon://",              },
  { "local", "local:///foo",       packeteer::CT_LOCAL, pnet::socket_address::SAT_LOCAL,  "local:///foo",         },
  { "pipe",  "pipe:///foo",        packeteer::CT_PIPE,  pnet::socket_address::SAT_LOCAL,  "pipe:///foo",          },

  // ports
  { "tcp4",  "tcp://192.168.0.1:1234", packeteer::CT_TCP4,  pnet::socket_address::SAT_INET4,  "tcp4://192.168.0.1:1234", },
  { "udp6",  "udp6://[::1]:4321",      packeteer::CT_UDP6,  pnet::socket_address::SAT_INET6,  "udp6://[::1]:4321",       },
};

} // anonymous namespace

class PeerAddressTest
    : public CppUnit::TestFixture
{
public:
  CPPUNIT_TEST_SUITE(PeerAddressTest);

    CPPUNIT_TEST(testStringConstruction);
    CPPUNIT_TEST(testValueSemantics);

  CPPUNIT_TEST_SUITE_END();

private:


  void testStringConstruction()
  {
    // Tests that information doesn't get mangled during construction or
    // formatting
    using namespace packeteer;

    for (size_t i = 0 ; i < sizeof(tests) / sizeof(test_data) ; ++i) {
      peer_address address(tests[i].address);

      CPPUNIT_ASSERT_EQUAL(tests[i].scheme,  address.scheme());
      CPPUNIT_ASSERT_EQUAL(tests[i].sa_type, address.socket_address().type());
      CPPUNIT_ASSERT_EQUAL(tests[i].type, address.conn_type());
      CPPUNIT_ASSERT_EQUAL(tests[i].expected, address.str());
    }
  }

  void testValueSemantics()
  {
    using namespace packeteer;

    // TCP4 and TCP with an IPv4 address should be equivalent.
    PACKETEER_VALUES_TEST(peer_address("tcp4://192.168.0.1"),
                          peer_address("tcp://192.168.0.1"),
                          true);

    // However, different IPs should be non-equal
    PACKETEER_VALUES_TEST(peer_address("tcp4://192.168.0.1"),
                          peer_address("tcp4://192.168.0.2"),
                          false);

    // And so should the same IP with different protocols
    PACKETEER_VALUES_TEST(peer_address("tcp4://192.168.0.1"),
                          peer_address("udp4://192.168.0.1"),
                          false);
  }
};


CPPUNIT_TEST_SUITE_REGISTRATION(PeerAddressTest);
