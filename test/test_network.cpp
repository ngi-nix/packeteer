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

#include <packeteer/net/network.h>
#include <packeteer/net/socket_address.h>

#include <cppunit/extensions/HelperMacros.h>

#include <cstring>

#include <sstream>
#include <string>

namespace pnet = packeteer::net;

namespace {

// Ctor tests
struct ctor_test_data
{
  char const *  netspec;
  bool          throws;
  sa_family_t   expected_proto;
  size_t        expected_mask;
  char const *  expected_network;
  char const *  expected_broadcast;
} ctor_tests[] = {
  // Garbage
  { "asddfs",         true,   AF_UNSPEC, size_t(-1), "", "", },
  { "asddfs",         true,   AF_UNSPEC, size_t(-1), "", "", },

  // IPv4 hosts
  { "192.168.0.1",    true,   AF_UNSPEC, size_t(-1), "", "", },

  // IPv4 networks
  { "192.168.0.1/33", true,       AF_INET,   size_t(-1), "", "", },
  { "192.168.0.1/32", false,      AF_INET,   32, "192.168.0.1",     "192.168.0.1", },
  { "192.168.134.121/31", false,  AF_INET,   31, "192.168.134.120", "192.168.134.121", },
  { "192.168.134.121/25", false,  AF_INET,   25, "192.168.134.0",   "192.168.134.127", },
  { "192.168.134.121/24", false,  AF_INET,   24, "192.168.134.0",   "192.168.134.255", },
  { "192.168.134.121/23", false,  AF_INET,   23, "192.168.134.0",   "192.168.135.255", },
  { "192.168.134.121/17", false,  AF_INET,   17, "192.168.128.0",   "192.168.255.255", },
  { "192.168.134.121/16", false,  AF_INET,   16, "192.168.0.0",     "192.168.255.255", },
  { "192.168.134.121/15", false,  AF_INET,   15, "192.168.0.0",     "192.169.255.255", },
  { "192.168.134.121/9",  false,  AF_INET,    9, "192.128.0.0",     "192.255.255.255", },
  { "192.168.134.121/8",  false,  AF_INET,    8, "192.0.0.0",       "192.255.255.255", },
  { "192.168.134.121/7",  false,  AF_INET,    7, "192.0.0.0",       "193.255.255.255", },
  { "192.168.134.121/0",  true,   AF_INET,   size_t(-1), "", "", },

  // IPv6 hosts
  { "2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true,   AF_UNSPEC, size_t(-1), "", "", },
  { "2001:0db8:85a3:0:0:8a2e:0370:7334",          true,   AF_UNSPEC, size_t(-1), "", "", },
  { "2001:0db8:85a3::8a2e:0370:7334",             true,   AF_UNSPEC, size_t(-1), "", "", },

  // IPv6 networks
  { "2001:0db8:85a3:0000:0000:8a2e:0370:7334/22", false,  AF_INET6,  22, "2001:C00::", "2001:fff:ffff:ffff:ffff:ffff:ffff:ffff", },
  { "2001:0db8:85a3:0:0:8a2e:0370:7334/22",       false,  AF_INET6,  22, "2001:C00::", "2001:fff:ffff:ffff:ffff:ffff:ffff:ffff", },
  { "2001:0db8:85a3::8a2e:0370:7334/22",          false,  AF_INET6,  22, "2001:C00::", "2001:fff:ffff:ffff:ffff:ffff:ffff:ffff", },

  { "2001:0db8:85a3:0000:0000:8a2e:0370:7334/129",true,   AF_INET6,  size_t(-1), "", "", },
  { "2001:0db8:85a3::8a2e:0370:7334/0",           true,   AF_INET6,  size_t(-1), "", "", },

};


// Contained tests
struct contained_test_data
{
  char const *  netspec;
  char const *  testee;
  bool          expected;
} contained_tests[] = {
  { "192.168.0.0/24",   "192.168.0.0",    true, },
  { "192.168.0.0/24",   "192.168.0.1",    true, },
  { "192.168.0.0/24",   "192.168.0.123",  true, },
  { "192.168.0.0/24",   "192.168.0.255",  true, },
  { "192.168.0.0/24",   "127.0.0.1",      false, },
  { "192.168.0.0/24",   "2001:C00::",     false, },
  { "2001:C00::/22",    "2001:C00::",                               true, },
  { "2001:C00::/22",    "2001:0db8:85a3:0000:0000:8a2e:0370:7334",  true, },
  { "2001:C00::/22",    "2001:fff:ffff:ffff:ffff:ffff:ffff:ffff",   true, },
  { "2001:C00::/22",    "2002:C00::",                               false, },
  { "2001:C00::/22",    "192.168.0.123",                            false, },
};

} // anonymous namespace

class NetworkTest
    : public CppUnit::TestFixture
{
public:
  CPPUNIT_TEST_SUITE(NetworkTest);

    CPPUNIT_TEST(testVerify);
    CPPUNIT_TEST(testConstruction);
    CPPUNIT_TEST(testInNetwork);
    CPPUNIT_TEST(testReset);
    CPPUNIT_TEST(testIPv4Allocation);
    CPPUNIT_TEST(testIDAllocation);
    CPPUNIT_TEST(testDirectAllocation);

  CPPUNIT_TEST_SUITE_END();

private:

  void testVerify()
  {
    using namespace pnet;

    for (size_t i = 0 ; i < sizeof(ctor_tests) / sizeof(ctor_test_data) ; ++i) {
      // std::cout << "Testing [" << ctor_tests[i].throws << "]: " << ctor_tests[i].netspec << std::endl;
      CPPUNIT_ASSERT_EQUAL(!ctor_tests[i].throws,
          network::verify_netspec(ctor_tests[i].netspec));
    }
  }



  void testReset()
  {
    using namespace pnet;

    // Simple test: create network, reserve an address from it.
    network net("192.168.0.1/24");
    socket_address address;
    CPPUNIT_ASSERT(!net.in_network(address));
    CPPUNIT_ASSERT_NO_THROW(address = net.reserve_address());
    CPPUNIT_ASSERT(net.in_network(address));

    // Now reset the network to a new range. The address can't be in_networK()
    // any longer.
    net.reset("10.0.0.0/8");
    CPPUNIT_ASSERT(!net.in_network(address));

    // Reserve a new address and things should be fine again.
    CPPUNIT_ASSERT_NO_THROW(address = net.reserve_address());
    CPPUNIT_ASSERT(net.in_network(address));
  }



  void testConstruction()
  {
    using namespace pnet;

    for (size_t i = 0 ; i < sizeof(ctor_tests) / sizeof(ctor_test_data) ; ++i) {
      // std::cout << "Testing [" << ctor_tests[i].throws << "]: " << ctor_tests[i].netspec << std::endl;
      if (ctor_tests[i].throws) {
        CPPUNIT_ASSERT_THROW(network n(ctor_tests[i].netspec), std::runtime_error);
      }
      else {
        network n(ctor_tests[i].netspec);
        CPPUNIT_ASSERT_EQUAL(ctor_tests[i].expected_proto, n.family());
        CPPUNIT_ASSERT_EQUAL(ctor_tests[i].expected_mask, n.mask_size());
        CPPUNIT_ASSERT_EQUAL(socket_address(ctor_tests[i].expected_network),
            n.network_address());
        CPPUNIT_ASSERT_EQUAL(socket_address(ctor_tests[i].expected_broadcast, UINT16_MAX),
            n.broadcast_address());
      }
    }
  }


  void testInNetwork()
  {
    using namespace pnet;

    for (size_t i = 0 ; i < sizeof(contained_tests) / sizeof(contained_test_data) ; ++i) {
      // std::cout << "Testing: " << contained_tests[i].netspec << " ("
      //  << contained_tests[i].testee << ")" << std::endl;
      network n(contained_tests[i].netspec);
      CPPUNIT_ASSERT_EQUAL(contained_tests[i].expected,
        n.in_network(socket_address(contained_tests[i].testee)));
    }
  }


  void testIPv4Allocation()
  {
    using namespace pnet;

    // The network has 14 available addresses - the network address and the
    // broadcast address don't count.
    network n("192.168.1.0/28");

    // Generate 14 addresses. Each of those must succeed, and each of those
    // must be unique.
    std::vector<std::shared_ptr<socket_address>> known;
    for (size_t i = 0 ; i < 14 ; ++i) {
      socket_address addr;
      CPPUNIT_ASSERT_NO_THROW(addr = n.reserve_address());

      for (size_t j = 0 ; j < known.size() ; ++j) {
        socket_address & k = *(known[j].get());
        CPPUNIT_ASSERT(!(addr == k));
      }
    }

    // The next allocation attempt should fail, though.
    CPPUNIT_ASSERT_THROW(auto addr = n.reserve_address(), std::runtime_error);

    // If we erase one address, and re-add it, that should work.
    CPPUNIT_ASSERT(n.release_address(socket_address("192.168.1.7")));
    socket_address addr;
    CPPUNIT_ASSERT_NO_THROW(addr = n.reserve_address());
    CPPUNIT_ASSERT_EQUAL(socket_address("192.168.1.7"), addr);

    // Erasing an unknown address should fail
    CPPUNIT_ASSERT(!n.release_address(socket_address("127.0.0.1")));

    // Lastly, erasing any of the known addresses must succeed.
    auto end = known.end();
    for (auto iter = known.begin() ; iter != end ; ++iter) {
      CPPUNIT_ASSERT(*iter);
      CPPUNIT_ASSERT(n.release_address(*iter->get()));
    }
  }



  void testIDAllocation()
  {
    using namespace pnet;

    // Creating a /24 network means there are only 254 available
    // addresses.
    network net("192.168.0.1/24");
    // std::cout << "MAX: " << net.max_size() << std::endl;

    // Let's allocate one first, and test that releasing and re-allocating
    // works (i.e. same ID gets same address).
    std::string id1 = "foobar";
    socket_address address;
    CPPUNIT_ASSERT_NO_THROW(address = net.reserve_address(id1));
    CPPUNIT_ASSERT(net.in_network(address));

    // XXX as *luck* would have it, "foobar" matches the following address,
    // given the current hash implementaiton.
    CPPUNIT_ASSERT_EQUAL(socket_address("192.168.0.98"), address);

    // Let's ensure first that using the same ID again will result in an
    // error result as it's already allocated.
    socket_address address2;
    CPPUNIT_ASSERT_THROW(address2 = net.reserve_address(id1), std::runtime_error);

    // However, releasing the address means we can get it again.
    net.release_address(address);
    CPPUNIT_ASSERT_NO_THROW(address2 = net.reserve_address(id1));
    CPPUNIT_ASSERT(net.in_network(address2));
    CPPUNIT_ASSERT_EQUAL(address, address2);

    // Right, now verify that another ID does not produce a collision.
    std::string id2 = "foobaz";
    CPPUNIT_ASSERT(id1 != id2);

    socket_address address3;
    CPPUNIT_ASSERT_NO_THROW(address3 = net.reserve_address(id2));
    CPPUNIT_ASSERT(net.in_network(address3));

    CPPUNIT_ASSERT(address2 != address3);
  }



  void testDirectAllocation()
  {
    using namespace pnet;

    // Creating a /24 network means there are only 254 available
    // addresses.
    network net("192.168.0.1/24");

    // Try to allocate a socket_address directly.
    CPPUNIT_ASSERT(net.reserve_address(socket_address("192.168.0.1")));

    // The same again won't work.
    CPPUNIT_ASSERT(!net.reserve_address(socket_address("192.168.0.1")));

    // But after releasing it will.
    CPPUNIT_ASSERT(net.release_address(socket_address("192.168.0.1")));
    CPPUNIT_ASSERT(net.reserve_address(socket_address("192.168.0.1")));

    // Reserving outside of the network will fail.
    CPPUNIT_ASSERT(!net.reserve_address(socket_address("10.0.0.1")));
  }
};


CPPUNIT_TEST_SUITE_REGISTRATION(NetworkTest);
