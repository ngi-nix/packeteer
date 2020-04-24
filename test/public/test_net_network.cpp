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

#include <packeteer/net/network.h>

// For AF_UNSPEC, etc.
#include "../lib/net/netincludes.h"

#include <gtest/gtest.h>

#include <cstring>

#include <sstream>
#include <string>

#include "../test_name.h"

namespace pnet = packeteer::net;

/*****************************************************************************
 * NetworkConstruction
 */

namespace {

// Ctor tests
struct ctor_test_data
{
  char const *        netspec;
  bool                throws;
  pnet::address_type  expected_proto;
  size_t              expected_mask;
  char const *        expected_network;
  char const *        expected_broadcast;
} ctor_tests[] = {
  // Garbage
  { "asddfs",         true,   pnet::AT_UNSPEC, size_t(-1), "", "", },

  // IPv4 hosts
  { "192.168.0.1",    true,   pnet::AT_UNSPEC, size_t(-1), "", "", },

  // IPv4 networks
  { "192.168.0.1/33", true,       pnet::AT_INET4,   size_t(-1), "", "", },
  { "192.168.0.1/32", false,      pnet::AT_INET4,   32, "192.168.0.1",     "192.168.0.1", },
  { "192.168.134.121/31", false,  pnet::AT_INET4,   31, "192.168.134.120", "192.168.134.121", },
  { "192.168.134.121/25", false,  pnet::AT_INET4,   25, "192.168.134.0",   "192.168.134.127", },
  { "192.168.134.121/24", false,  pnet::AT_INET4,   24, "192.168.134.0",   "192.168.134.255", },
  { "192.168.134.121/23", false,  pnet::AT_INET4,   23, "192.168.134.0",   "192.168.135.255", },
  { "192.168.134.121/17", false,  pnet::AT_INET4,   17, "192.168.128.0",   "192.168.255.255", },
  { "192.168.134.121/16", false,  pnet::AT_INET4,   16, "192.168.0.0",     "192.168.255.255", },
  { "192.168.134.121/15", false,  pnet::AT_INET4,   15, "192.168.0.0",     "192.169.255.255", },
  { "192.168.134.121/9",  false,  pnet::AT_INET4,    9, "192.128.0.0",     "192.255.255.255", },
  { "192.168.134.121/8",  false,  pnet::AT_INET4,    8, "192.0.0.0",       "192.255.255.255", },
  { "192.168.134.121/7",  false,  pnet::AT_INET4,    7, "192.0.0.0",       "193.255.255.255", },
  { "192.168.134.121/0",  true,   pnet::AT_INET4,   size_t(-1), "", "", },

  // IPv6 hosts
  { "2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true,   pnet::AT_UNSPEC, size_t(-1), "", "", },
  { "2001:0db8:85a3:0:0:8a2e:0370:7334",          true,   pnet::AT_UNSPEC, size_t(-1), "", "", },
  { "2001:0db8:85a3::8a2e:0370:7334",             true,   pnet::AT_UNSPEC, size_t(-1), "", "", },

  // IPv6 networks
  { "2001:0db8:85a3:0000:0000:8a2e:0370:7334/22", false,  pnet::AT_INET6,  22, "2001:C00::", "2001:fff:ffff:ffff:ffff:ffff:ffff:ffff", },
  { "2001:0db8:85a3:0:0:8a2e:0370:7334/22",       false,  pnet::AT_INET6,  22, "2001:C00::", "2001:fff:ffff:ffff:ffff:ffff:ffff:ffff", },
  { "2001:0db8:85a3::8a2e:0370:7334/22",          false,  pnet::AT_INET6,  22, "2001:C00::", "2001:fff:ffff:ffff:ffff:ffff:ffff:ffff", },

  { "2001:0db8:85a3:0000:0000:8a2e:0370:7334/129",true,   pnet::AT_INET6,  size_t(-1), "", "", },
  { "2001:0db8:85a3::8a2e:0370:7334/0",           true,   pnet::AT_INET6,  size_t(-1), "", "", },
};


std::string ctor_name(testing::TestParamInfo<ctor_test_data> const & info)
{
  return symbolize_name(info.param.netspec);
}


} // anonymous namespace

class NetworkConstruction
  : public testing::TestWithParam<ctor_test_data>
{
};


TEST_P(NetworkConstruction, verify)
{
  using namespace pnet;
  auto td = GetParam();

  ASSERT_NE(td.throws, network::verify_netspec(td.netspec));
}


TEST_P(NetworkConstruction, construct)
{
  using namespace pnet;
  auto td = GetParam();

  if (td.throws) {
    ASSERT_THROW(network n{td.netspec}, std::runtime_error);
  }
  else {
    network n{td.netspec};
    ASSERT_EQ(td.expected_proto, n.family());
    ASSERT_EQ(td.expected_mask, n.mask_size());
    ASSERT_EQ(socket_address(td.expected_network),
        n.network_address());
    ASSERT_EQ(socket_address(td.expected_broadcast, UINT16_MAX),
        n.broadcast_address());
  }
}

INSTANTIATE_TEST_SUITE_P(net, NetworkConstruction,
    testing::ValuesIn(ctor_tests),
    ctor_name);


/*****************************************************************************
 * NetworkContents
 */


namespace {

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



std::string contained_name(testing::TestParamInfo<contained_test_data> const & info)
{
  std::string name{info.param.netspec};
  name += "_";
  name += info.param.testee;

  return symbolize_name(name);
}



} // anonymous namespace


class NetworkContents
  : public testing::TestWithParam<contained_test_data>
{
};


TEST_P(NetworkContents, ip_in_network)
{
  using namespace pnet;
  auto td = GetParam();

  // std::cout << "Testing: " << td.netspec << " ("
  //  << td.testee << ")" << std::endl;
  network n{td.netspec};
  ASSERT_EQ(td.expected, n.in_network(socket_address(td.testee)));
}


INSTANTIATE_TEST_SUITE_P(net, NetworkContents,
    testing::ValuesIn(contained_tests),
    contained_name);



/*****************************************************************************
 * Network
 */


TEST(Network, reset)
{
  using namespace pnet;

  // Simple test: create network, reserve an address from it.
  network net{"192.168.0.1/24"};
  socket_address address;
  ASSERT_FALSE(net.in_network(address));
  ASSERT_NO_THROW(address = net.reserve_address());
  ASSERT_TRUE(net.in_network(address));

  // Now reset the network to a new range. The address can't be in_networK()
  // any longer.
  net.reset("10.0.0.0/8");
  ASSERT_FALSE(net.in_network(address));

  // Reserve a new address and things should be fine again.
  ASSERT_NO_THROW(address = net.reserve_address());
  ASSERT_TRUE(net.in_network(address));
}


TEST(Network, ipv4_allocation)
{
  using namespace pnet;

  // The network has 14 available addresses - the network address and the
  // broadcast address don't count.
  network n{"192.168.1.0/28"};

  // Generate 14 addresses. Each of those must succeed, and each of those
  // must be unique.
  std::vector<std::shared_ptr<socket_address>> known;
  for (size_t i = 0 ; i < 14 ; ++i) {
    socket_address addr;
    ASSERT_NO_THROW(addr = n.reserve_address());

    for (size_t j = 0 ; j < known.size() ; ++j) {
      socket_address & k = *(known[j].get());
      ASSERT_NE(addr, k);
    }
  }

  // The next allocation attempt should fail, though.
  ASSERT_THROW(auto addr = n.reserve_address(), std::runtime_error);

  // If we erase one address, and re-add it, that should work.
  EXPECT_TRUE(n.release_address(socket_address("192.168.1.7")));
  socket_address addr;
  ASSERT_NO_THROW(addr = n.reserve_address());
  ASSERT_EQ(socket_address("192.168.1.7"), addr);

  // Erasing an unknown address should fail
  ASSERT_FALSE(n.release_address(socket_address("127.0.0.1")));

  // Lastly, erasing any of the known addresses must succeed.
  auto end = known.end();
  for (auto iter = known.begin() ; iter != end ; ++iter) {
    EXPECT_TRUE(*iter);
    ASSERT_TRUE(n.release_address(*iter->get()));
  }
}


TEST(Network, ipv4_allocation_with_id)
{
  using namespace pnet;

  // Creating a /24 network means there are only 254 available
  // addresses.
  network net{"192.168.0.1/24"};
  // std::cout << "MAX: " << net.max_size() << std::endl;

  // Let's allocate one first, and test that releasing and re-allocating
  // works (i.e. same ID gets same address).
  std::string id1 = "foobar";
  socket_address address;
  ASSERT_NO_THROW(address = net.reserve_address(id1));
  ASSERT_TRUE(net.in_network(address));

  // Let's ensure first that using the same ID again will result in an
  // error result as it's already allocated.
  socket_address address2;
  ASSERT_THROW(address2 = net.reserve_address(id1), std::runtime_error);

  // However, releasing the address means we can get it again.
  net.release_address(address);
  ASSERT_NO_THROW(address2 = net.reserve_address(id1));
  ASSERT_TRUE(net.in_network(address2));
  ASSERT_EQ(address, address2);

  // Right, now verify that another ID does not produce a collision.
  std::string id2 = "foobaz";
  ASSERT_NE(id1, id2);

  socket_address address3;
  ASSERT_NO_THROW(address3 = net.reserve_address(id2));
  ASSERT_TRUE(net.in_network(address3));

  ASSERT_NE(address2, address3);
}


TEST(Network, direct_allocation)
{
  using namespace pnet;

  // Creating a /24 network means there are only 254 available
  // addresses.
  network net{"192.168.0.1/24"};

  // Try to allocate a socket_address directly.
  ASSERT_TRUE(net.reserve_address(socket_address("192.168.0.1")));

  // The same again won't work.
  ASSERT_FALSE(net.reserve_address(socket_address("192.168.0.1")));

  // But after releasing it will.
  EXPECT_TRUE(net.release_address(socket_address("192.168.0.1")));
  ASSERT_TRUE(net.reserve_address(socket_address("192.168.0.1")));

  // Reserving outside of the network will fail.
  ASSERT_FALSE(net.reserve_address(socket_address("10.0.0.1")));
}
