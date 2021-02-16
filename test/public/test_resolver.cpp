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


TEST(Resolver, custom_scheme_fails_without_registration)
{
  auto api = packeteer::api::create();
  auto url = l6e::net::url::parse("test-scheme:///foo/bar");
  std::set<l6e::net::url> results;

  // Without registerting anything, a test url won't be resolved.
  auto err = api->resolver().resolve(results, url);
  ASSERT_EQ(p7r::ERR_INVALID_VALUE, err);
}



TEST(Resolver, custom_scheme_works_with_registration)
{
  auto api = packeteer::api::create();
  auto url = l6e::net::url::parse("test-scheme:///foo/bar");
  std::set<l6e::net::url> results;

  // Register a test resolution function, try again.
  auto err = api->resolver().register_resolution_function("test-scheme",
      [](std::shared_ptr<p7r::api>, std::set<l6e::net::url> & res, l6e::net::url const & query) -> p7r::error_t
      {
        auto copy = query;
        copy.path.replace(1, 3, "quux");
        res.insert(copy);
        return p7r::ERR_SUCCESS;
      }
  );
  ASSERT_EQ(p7r::ERR_SUCCESS, err);
  err = api->resolver().resolve(results, url);
  ASSERT_EQ(p7r::ERR_SUCCESS, err);
  ASSERT_EQ(1, results.size());
  auto res = *results.begin();
  ASSERT_EQ(res.path, "/quux/bar");
}


TEST(Resolver, custom_scheme_double_registration_fails)
{
  auto api = packeteer::api::create();

  auto err = api->resolver().register_resolution_function("test-scheme",
      [](std::shared_ptr<p7r::api>, std::set<l6e::net::url> &, l6e::net::url const &) -> p7r::error_t
      {
        return p7r::ERR_SUCCESS;
      }
  );
  ASSERT_EQ(p7r::ERR_SUCCESS, err);


  // Registering the same scheme again will fail.
  err = api->resolver().register_resolution_function("test-scheme",
      [](std::shared_ptr<p7r::api>, std::set<l6e::net::url> &, l6e::net::url const &) -> p7r::error_t
      {
        return p7r::ERR_SUCCESS;
      }
  );
  ASSERT_EQ(p7r::ERR_INVALID_VALUE, err);
}


TEST(Resolver, custom_scheme_produces_errors)
{
  auto api = packeteer::api::create();
  auto url = l6e::net::url::parse("error:///foo/bar");
  std::set<l6e::net::url> results;

  // Registering an erroring scheme, and try to resolve that.
  auto err = api->resolver().register_resolution_function("error",
      [](std::shared_ptr<p7r::api>, std::set<l6e::net::url> &, l6e::net::url const &) -> p7r::error_t
      {
        // Actually, very expected.
        return p7r::ERR_UNEXPECTED;
      }
  );
  ASSERT_EQ(p7r::ERR_SUCCESS, err);

  err = api->resolver().resolve(results, url);
  ASSERT_EQ(p7r::ERR_UNEXPECTED, err);
}
