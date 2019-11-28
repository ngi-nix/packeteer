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

#include <packeteer/connector/peer_address.h>

#include <gtest/gtest.h>

#include <cstring>

#include <sstream>
#include <string>
#include <set>

#include "../value_tests.h"
#include "../test_name.h"

namespace pnet = packeteer::net;

namespace {

struct test_data
{
  std::string                 scheme;
  std::string                 address;
  packeteer::connector_type   type;
  pnet::address_type          sa_type;
  std::string                 expected;
} tests[] = {
  // All schemes, simple.
  { "tcp4",  "tcp4://192.168.0.1", packeteer::CT_TCP4,  pnet::AT_INET4,  "tcp4://192.168.0.1:0", },
  { "tcp4",  "tcp://192.168.0.1",  packeteer::CT_TCP4,  pnet::AT_INET4,  "tcp4://192.168.0.1:0", },
  { "tcp6",  "tcp6://::1",         packeteer::CT_TCP6,  pnet::AT_INET6,  "tcp6://[::1]:0",       },
  { "tcp6",  "tcp://::1",          packeteer::CT_TCP6,  pnet::AT_INET6,  "tcp6://[::1]:0",       },
  { "udp4",  "udp4://192.168.0.1", packeteer::CT_UDP4,  pnet::AT_INET4,  "udp4://192.168.0.1:0", },
  { "udp4",  "udp://192.168.0.1",  packeteer::CT_UDP4,  pnet::AT_INET4,  "udp4://192.168.0.1:0", },
  { "udp6",  "udp6://::1",         packeteer::CT_UDP6,  pnet::AT_INET6,  "udp6://[::1]:0",       },
  { "udp6",  "udp://::1",          packeteer::CT_UDP6,  pnet::AT_INET6,  "udp6://[::1]:0",       },
  { "anon",  "anon://",            packeteer::CT_ANON,  pnet::AT_UNSPEC, "anon://",              },
#if defined(PACKETEER_POSIX)
  { "pipe",  "pipe:///foo",        packeteer::CT_PIPE,  pnet::AT_LOCAL,  "pipe:///foo",          },
  { "local", "local:///foo",       packeteer::CT_LOCAL, pnet::AT_LOCAL,  "local:///foo",         },
#endif

  // ports
  { "tcp4",  "tcp://192.168.0.1:1234", packeteer::CT_TCP4,  pnet::AT_INET4,  "tcp4://192.168.0.1:1234", },
  { "udp6",  "udp6://[::1]:4321",      packeteer::CT_UDP6,  pnet::AT_INET6,  "udp6://[::1]:4321",       },
};


std::string generate_name(testing::TestParamInfo<test_data> const & info)
{
  return symbolize_name(info.param.address);
}

} // anonymous namespace


class PeerAddress
  : public testing::TestWithParam<test_data>
{
};


TEST_P(PeerAddress, string_construction)
{
  auto td = GetParam();

  // Tests that information doesn't get mangled during construction or
  // formatting
  using namespace packeteer;

  peer_address address(td.address);

  ASSERT_EQ(td.scheme,  address.scheme());
  ASSERT_EQ(td.sa_type, address.socket_address().type());
  ASSERT_EQ(td.type, address.conn_type());
  ASSERT_EQ(td.expected, address.str());
}


INSTANTIATE_TEST_CASE_P(net, PeerAddress,
    testing::ValuesIn(tests),
    generate_name);


TEST(PeerAddressValueSemantics, expanded_scheme)
{
  using namespace packeteer;

  peer_address first{"tcp4://192.168.0.1"};
  peer_address second{"tcp://192.168.0.1"};

  test_copy_construction(first);
  test_assignment(first);

  test_equality(first, second);
  test_hashing_equality(first, second);
}


TEST(PeerAddressValueSemantics, different_address)
{
  using namespace packeteer;

  peer_address first{"tcp4://192.168.0.1"};
  peer_address second{"tcp4://192.168.0.2"};

  test_less_than(first, second);
  test_hashing_inequality(first, second);
}


TEST(PeerAddressValueSemantics, different_protocol)
{
  using namespace packeteer;

  peer_address first{"tcp4://192.168.0.1"};
  peer_address second{"udp4://192.168.0.1"};

  test_less_than(first, second);
  test_hashing_inequality(first, second);
}
