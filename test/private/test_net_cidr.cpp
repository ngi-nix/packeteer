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

#include <cstring>

#include <sstream>
#include <string>
#include <set>

#include <gtest/gtest.h>

#include "../../lib/net/cidr.h"

#include "../test_name.h"

namespace pnet = packeteer::net;

namespace {

using namespace packeteer;

struct test_data
{
  char const *        netspec;
  bool                no_mask;
  packeteer::error_t  expected_error;
  sa_family_t         expected_proto;
  ssize_t             expected_mask;
  uint16_t            port;
  uint16_t            expected_port1;
  uint16_t            expected_port2;
} tests[] = {
  // Garbage (except for port)
  { "asddfs",         true,   ERR_ABORTED, AF_UNSPEC, -1, 12345, 0, 12345 },
  { "asddfs",         false,  ERR_ABORTED, AF_UNSPEC, -1, 12345, 0, 12345 },

  // IPv4 hosts
  { "192.168.0.1",    true,   ERR_SUCCESS, AF_INET,    0, 12345, 0, 12345 },
  { "192.168.0.1/24", true,   ERR_INVALID_VALUE, AF_UNSPEC, -1, 12345, 0, 12345 },

  // IPv4 hosts with port
  { "192.168.0.1:22",    false,  ERR_INVALID_VALUE, AF_INET,    -1, 0, 22, 22 },
  { "192.168.0.1:22",    false,  ERR_INVALID_VALUE, AF_INET,    -1, 12345, 22, 12345 },
  { "192.168.0.1:xx",    false,  ERR_ABORTED, AF_UNSPEC,  -1, 0, 0, 0 },
  { "192.168.0.1:22/24", false,  ERR_INVALID_VALUE, AF_UNSPEC,  -1, 0, 0, 0 },

  // IPv4 networks
  { "192.168.0.1/33", false,  ERR_INVALID_VALUE, AF_INET,   -1, 12345, 0, 12345 },
  { "192.168.0.1/32", false,  ERR_SUCCESS, AF_INET,   32, 12345, 0, 12345 },
  { "192.168.0.1/31", false,  ERR_SUCCESS, AF_INET,   31, 12345, 0, 12345 },
  { "192.168.0.1/25", false,  ERR_SUCCESS, AF_INET,   25, 12345, 0, 12345 },
  { "192.168.0.1/24", false,  ERR_SUCCESS, AF_INET,   24, 12345, 0, 12345 },
  { "192.168.0.1/23", false,  ERR_SUCCESS, AF_INET,   23, 12345, 0, 12345 },
  { "192.168.0.1/17", false,  ERR_SUCCESS, AF_INET,   17, 12345, 0, 12345 },
  { "192.168.0.1/16", false,  ERR_SUCCESS, AF_INET,   16, 12345, 0, 12345 },
  { "192.168.0.1/15", false,  ERR_SUCCESS, AF_INET,   15, 12345, 0, 12345 },
  { "192.168.0.1/8",  false,  ERR_SUCCESS, AF_INET,    8, 12345, 0, 12345 },
  { "192.168.0.1/7",  false,  ERR_SUCCESS, AF_INET,    7, 12345, 0, 12345 },
  { "192.168.0.1/0",  false,  ERR_INVALID_VALUE, AF_INET,   -1, 12345, 0, 12345 },

  // IPv6 hosts
  { "2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true,   ERR_SUCCESS, AF_INET6,   0, 12345, 0, 12345 },
  { "2001:0db8:85a3:0:0:8a2e:0370:7334",          true,   ERR_SUCCESS, AF_INET6,   0, 12345, 0, 12345 },
  { "2001:0db8:85a3::8a2e:0370:7334",             true,   ERR_SUCCESS, AF_INET6,   0, 12345, 0, 12345 },
  { "2001:0db8:85a3:0000:0000:8a2e:0370:7334/10", true,   ERR_INVALID_VALUE, AF_UNSPEC, -1, 12345, 0, 12345 },
  { "2001:0db8:85a3:0:0:8a2e:0370:7334/10",       true,   ERR_INVALID_VALUE, AF_UNSPEC, -1, 12345, 0, 12345 },
  { "2001:0db8:85a3::8a2e:0370:7334/10",          true,   ERR_INVALID_VALUE, AF_UNSPEC, -1, 12345, 0, 12345 },

  // IPv6 hosts with port
  { "[2001:0db8:85a3::8a2e:0370:7334]:22",        false,   ERR_INVALID_VALUE, AF_INET6,  -1, 12345, 22, 12345 },
  { "[2001:0db8:85a3::8a2e:0370:7334]:22",        false,   ERR_INVALID_VALUE, AF_INET6,  -1, 0, 22, 22 },
  { "[2001:0db8:85a3::8a2e:0370:7334",            false,   ERR_ABORTED, AF_UNSPEC, -1, 0, 0, 0 },
  { "[2001:0db8:85a3::8a2e:0370:7334]:ab",        false,   ERR_ABORTED, AF_UNSPEC, -1, 0, 0, 0 },
  { "[2001:0db8:85a3::8a2e:0370:7334]:22/24",     false,   ERR_INVALID_VALUE, AF_UNSPEC, -1, 0, 0, 0 },

  // IPv6 networks
  { "2001:0db8:85a3:0000:0000:8a2e:0370:7334/22", false,  ERR_SUCCESS, AF_INET6,  22, 12345, 0, 12345 },
  { "2001:0db8:85a3:0:0:8a2e:0370:7334/22",       false,  ERR_SUCCESS, AF_INET6,  22, 12345, 0, 12345 },
  { "2001:0db8:85a3::8a2e:0370:7334/22",          false,  ERR_SUCCESS, AF_INET6,  22, 12345, 0, 12345 },

  { "2001:0db8:85a3:0000:0000:8a2e:0370:7334/129",false,  ERR_INVALID_VALUE, AF_INET6,  -1, 12345, 0, 12345 },
  { "2001:0db8:85a3::8a2e:0370:7334/0",           false,  ERR_INVALID_VALUE, AF_INET6,  -1, 12345, 0, 12345 },

};


std::string generate_name(testing::TestParamInfo<test_data> const & info)
{
  std::string name{ info.param.netspec };

  name += "_";
  name += (info.param.no_mask ? "no_mask" : "mask");

  name += "_port_";
  name += std::to_string(info.param.port);

  return symbolize_name(name);
}

} // anonymous namespace


class CIDR
  : public testing::TestWithParam<test_data>
{
};


TEST_P(CIDR, parsing_without_explicit_port)
{
  auto td = GetParam();

  // std::cout << "=== test: " << td.netspec << std::endl;

  // Parse without specifying port
  pnet::detail::address_data address;
  pnet::detail::parse_result_t result(address);
  packeteer::error_t err = pnet::detail::parse_extended_cidr(td.netspec,
      td.no_mask, result);
  ASSERT_EQ(td.expected_error, err);

  ASSERT_EQ(td.expected_proto, result.proto);
  ASSERT_EQ(td.expected_mask, result.mask);

  if (AF_INET == result.proto) {
    sockaddr_in * addr = reinterpret_cast<sockaddr_in *>(&result.address);
    ASSERT_EQ(uint16_t(htons(td.expected_port1)), addr->sin_port);
  }
  else if (AF_INET6 == result.proto) {
    sockaddr_in6 * addr = reinterpret_cast<sockaddr_in6 *>(&result.address);
    ASSERT_EQ(uint16_t(htons(td.expected_port1)), addr->sin6_port);
  }
}


TEST_P(CIDR, parsing_with_explicit_port)
{
  auto td = GetParam();

  // std::cout << "=== test: " << td.netspec << std::endl;

  // Parsing the same thing again with port should change the test
  // results a little.
  pnet::detail::address_data address;
  pnet::detail::parse_result_t result(address);
  ::memset(&address, 0, sizeof(sockaddr_storage));
  packeteer::error_t err = pnet::detail::parse_extended_cidr(td.netspec,
      td.no_mask, result, td.port);
  ASSERT_EQ(td.expected_error, err);

  ASSERT_EQ(td.expected_proto, result.proto);
  ASSERT_EQ(td.expected_mask, result.mask);

  if (AF_INET == result.proto) {
    sockaddr_in * addr = reinterpret_cast<sockaddr_in *>(&result.address);
    ASSERT_EQ(uint16_t(htons(td.expected_port2)), addr->sin_port);
  }
  else if (AF_INET6 == result.proto) {
    sockaddr_in6 * addr = reinterpret_cast<sockaddr_in6 *>(&result.address);
    ASSERT_EQ(uint16_t(htons(td.expected_port2)), addr->sin6_port);
  }
}


INSTANTIATE_TEST_CASE_P(net, CIDR, testing::ValuesIn(tests),
    generate_name);
