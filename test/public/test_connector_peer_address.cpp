/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2017-2020 Jens Finkhaeuser.
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

namespace net = liberate::net;

namespace {

struct test_data
{
  std::string                 scheme;
  std::string                 url_string;
  packeteer::connector_type   type;
  net::address_type           sa_type;
  std::string                 expected;
} tests[] = {
  // All schemes, simple.
  { "tcp4",  "tcp4://192.168.0.1", packeteer::CT_TCP4,  net::AT_INET4,  "tcp4://192.168.0.1:0", },
  { "tcp4",  "tcp://192.168.0.1",  packeteer::CT_TCP4,  net::AT_INET4,  "tcp4://192.168.0.1:0", },
  { "tcp6",  "tcp6://::1",         packeteer::CT_TCP6,  net::AT_INET6,  "tcp6://[::1]:0",       },
  { "tcp6",  "tcp://::1",          packeteer::CT_TCP6,  net::AT_INET6,  "tcp6://[::1]:0",       },
  { "udp4",  "udp4://192.168.0.1", packeteer::CT_UDP4,  net::AT_INET4,  "udp4://192.168.0.1:0", },
  { "udp4",  "udp://192.168.0.1",  packeteer::CT_UDP4,  net::AT_INET4,  "udp4://192.168.0.1:0", },
  { "udp6",  "udp6://::1",         packeteer::CT_UDP6,  net::AT_INET6,  "udp6://[::1]:0",       },
  { "udp6",  "udp://::1",          packeteer::CT_UDP6,  net::AT_INET6,  "udp6://[::1]:0",       },
  { "anon",  "anon://",            packeteer::CT_ANON,  net::AT_UNSPEC, "anon://",              },
  { "local", "local:///foo",       packeteer::CT_LOCAL, net::AT_LOCAL,  "local:///foo",         },
  { "local", "local://",           packeteer::CT_LOCAL, net::AT_UNSPEC, "local://",             },
  { "local", std::string{"local:///\0abstract", 18}, packeteer::CT_LOCAL, net::AT_LOCAL, "local:///%00abstract", },
  { "local", "local:///%00abstract",                 packeteer::CT_LOCAL, net::AT_LOCAL, "local:///%00abstract", },

  // ports
  { "tcp4",  "tcp://192.168.0.1:1234", packeteer::CT_TCP4,  net::AT_INET4,  "tcp4://192.168.0.1:1234", },
  { "udp6",  "udp6://[::1]:4321",      packeteer::CT_UDP6,  net::AT_INET6,  "udp6://[::1]:4321",       },

#if defined(PACKETEER_WIN32)
  { "pipe",  "pipe:///foo",        packeteer::CT_PIPE,  net::AT_LOCAL,  "pipe:///foo",          },
#endif

#if defined(PACKETEER_POSIX)
  { "fifo",  "fifo:///foo",        packeteer::CT_FIFO,  net::AT_LOCAL,  "fifo:///foo",          },
#endif
};


std::string generate_name(testing::TestParamInfo<test_data> const & info)
{
  return symbolize_name(info.param.url_string);
}

} // anonymous namespace


class PeerAddress
  : public testing::TestWithParam<test_data>
{
};


TEST_P(PeerAddress, string_construction)
{
  auto td = GetParam();

  auto api = packeteer::api::create();

  // Tests that information doesn't get mangled during construction or
  // formatting
  using namespace packeteer;

  peer_address address;

  ASSERT_NO_THROW((address = peer_address{api, td.url_string}));

  ASSERT_EQ(td.scheme,  address.scheme());
  ASSERT_EQ(td.sa_type, address.socket_address().type());
  ASSERT_EQ(td.type, address.conn_type());
  ASSERT_EQ(td.expected, address.str());
}


INSTANTIATE_TEST_SUITE_P(net, PeerAddress,
    testing::ValuesIn(tests),
    generate_name);


TEST(PeerAddressValueSemantics, expanded_scheme)
{
  using namespace packeteer;

  auto api = packeteer::api::create();

  peer_address first{api, "tcp4://192.168.0.1"};
  peer_address second{api, "tcp://192.168.0.1"};

  test_copy_construction(first);
  test_assignment(first);

  test_equality(first, second);
  test_hashing_equality(first, second);
}


TEST(PeerAddressValueSemantics, different_address)
{
  using namespace packeteer;

  auto api = packeteer::api::create();

  peer_address first{api, "tcp4://192.168.0.1"};
  peer_address second{api, "tcp4://192.168.0.2"};

  test_less_than(first, second);
  test_hashing_inequality(first, second);
}


TEST(PeerAddressValueSemantics, different_protocol)
{
  using namespace packeteer;

  auto api = packeteer::api::create();

  peer_address first{api, "tcp4://192.168.0.1"};
  peer_address second{api, "udp4://192.168.0.1"};

  test_less_than(first, second);
  test_hashing_inequality(first, second);
}
