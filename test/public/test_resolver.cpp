/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2020 Jens Finkhaeuser.
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
#include "../env.h"

#include <packeteer.h>
#include <packeteer/resolver.h>

namespace p7r = packeteer;
namespace l6e = liberate;

TEST(Resolver, resolve_tcp4_with_ip)
{
  auto url = l6e::net::url::parse("tcp4://127.0.0.1:12345/foo/bar?quux=asdas");
  std::set<l6e::net::url> results;
  auto err = test_env->api->resolver().resolve(results, url);
  ASSERT_EQ(p7r::ERR_SUCCESS, err);

  ASSERT_EQ(1, results.size());
  auto resolved = *results.begin();
  ASSERT_EQ("tcp4", resolved.scheme);
  ASSERT_EQ("127.0.0.1:12345", resolved.authority);
  ASSERT_EQ("/foo/bar", resolved.path);
  ASSERT_EQ(1, resolved.query.size());
}



TEST(Resolver, resolve_udp4_with_ip)
{
  auto url = l6e::net::url::parse("udp4://127.0.0.1:12345/foo/bar?quux=asdas");
  std::set<l6e::net::url> results;
  auto err = test_env->api->resolver().resolve(results, url);
  ASSERT_EQ(p7r::ERR_SUCCESS, err);

  ASSERT_EQ(1, results.size());
  auto resolved = *results.begin();
  ASSERT_EQ("udp4", resolved.scheme);
  ASSERT_EQ("127.0.0.1:12345", resolved.authority);
  ASSERT_EQ("/foo/bar", resolved.path);
  ASSERT_EQ(1, resolved.query.size());
}



TEST(Resolver, resolve_tcp6_with_ip)
{
  auto url = l6e::net::url::parse("tcp6://[::1]:12345/foo/bar?quux=asdas");
  std::set<l6e::net::url> results;
  auto err = test_env->api->resolver().resolve(results, url);
  ASSERT_EQ(p7r::ERR_SUCCESS, err);

  ASSERT_EQ(1, results.size());
  auto resolved = *results.begin();
  ASSERT_EQ("tcp6", resolved.scheme);
  ASSERT_EQ("[::1]:12345", resolved.authority);
  ASSERT_EQ("/foo/bar", resolved.path);
  ASSERT_EQ(1, resolved.query.size());
}



TEST(Resolver, resolve_udp6_with_ip)
{
  auto url = l6e::net::url::parse("udp6://[::1]:12345/foo/bar?quux=asdas");
  std::set<l6e::net::url> results;
  auto err = test_env->api->resolver().resolve(results, url);
  ASSERT_EQ(p7r::ERR_SUCCESS, err);

  ASSERT_EQ(1, results.size());
  auto resolved = *results.begin();
  ASSERT_EQ("udp6", resolved.scheme);
  ASSERT_EQ("[::1]:12345", resolved.authority);
  ASSERT_EQ("/foo/bar", resolved.path);
  ASSERT_EQ(1, resolved.query.size());
}



TEST(Resolver, resolve_udp_with_ip4)
{
  auto url = l6e::net::url::parse("udp://127.0.0.1:12345/foo/bar?quux=asdas");
  std::set<l6e::net::url> results;
  auto err = test_env->api->resolver().resolve(results, url);
  ASSERT_EQ(p7r::ERR_SUCCESS, err);

  ASSERT_EQ(1, results.size());
  auto resolved = *results.begin();
  ASSERT_EQ("udp4", resolved.scheme);
  ASSERT_EQ("127.0.0.1:12345", resolved.authority);
  ASSERT_EQ("/foo/bar", resolved.path);
  ASSERT_EQ(1, resolved.query.size());
}



TEST(Resolver, resolve_tcp_with_ip6)
{
  auto url = l6e::net::url::parse("tcp://[::1]:12345/foo/bar?quux=asdas");
  std::set<l6e::net::url> results;
  auto err = test_env->api->resolver().resolve(results, url);
  ASSERT_EQ(p7r::ERR_SUCCESS, err);

  ASSERT_EQ(1, results.size());
  auto resolved = *results.begin();
  ASSERT_EQ("tcp6", resolved.scheme);
  ASSERT_EQ("[::1]:12345", resolved.authority);
  ASSERT_EQ("/foo/bar", resolved.path);
  ASSERT_EQ(1, resolved.query.size());
}



TEST(Resolver, resolve_tcp4_with_localhost)
{
  auto url = l6e::net::url::parse("tcp4://localhost:12345/foo/bar?quux=asdas");
  std::set<l6e::net::url> results;
  auto err = test_env->api->resolver().resolve(results, url);
  if (results.empty()) {
    GTEST_SKIP();
    return;
  }
  ASSERT_EQ(p7r::ERR_SUCCESS, err);

  ASSERT_EQ(1, results.size());
  auto resolved = *results.begin();
  ASSERT_EQ("tcp4", resolved.scheme);
  ASSERT_EQ("127.0.0.1:12345", resolved.authority);
  ASSERT_EQ("/foo/bar", resolved.path);
  ASSERT_EQ(1, resolved.query.size());
}



TEST(Resolver, resolve_udp6_with_localhost)
{
  auto url = l6e::net::url::parse("udp6://localhost:12345/foo/bar?quux=asdas");
  std::set<l6e::net::url> results;
  auto err = test_env->api->resolver().resolve(results, url);
  if (results.empty()) {
    GTEST_SKIP();
    return;
  }
  ASSERT_EQ(p7r::ERR_SUCCESS, err);

  ASSERT_EQ(1, results.size());
  auto resolved = *results.begin();
  ASSERT_EQ("udp6", resolved.scheme);
  ASSERT_EQ("[::1]:12345", resolved.authority);
  ASSERT_EQ("/foo/bar", resolved.path);
  ASSERT_EQ(1, resolved.query.size());
}



TEST(Resolver, resolve_udp_with_localhost)
{
  auto url = l6e::net::url::parse("udp://localhost:12345/foo/bar?quux=asdas");
  std::set<l6e::net::url> results;
  auto err = test_env->api->resolver().resolve(results, url);
  if (results.empty()) {
    GTEST_SKIP();
    return;
  }
  ASSERT_EQ(p7r::ERR_SUCCESS, err);

  ASSERT_TRUE(results.size() == 1 || results.size() == 2);

  for (auto res : results) {
    ASSERT_EQ("/foo/bar", res.path);
    ASSERT_EQ(1, res.query.size());

    ASSERT_TRUE(res.scheme == "udp6" || res.scheme == "udp4");
    if (res.scheme == "udp6") {
      ASSERT_EQ("[::1]:12345", res.authority);
    }
    else {
      ASSERT_EQ("127.0.0.1:12345", res.authority);
    }
  }
}

// TODO test cases:
// - udp_with_localhost -> 1-2, different types
