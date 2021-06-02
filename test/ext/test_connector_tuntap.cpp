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

#include <packeteer/ext/connector/tuntap.h>
#include <packeteer/connector.h>

namespace p7r = packeteer;

namespace {

inline auto
api_if_implemented()
{
  auto api = p7r::api::create();
  auto err = p7r::ext::register_connector_tuntap(api);
  if (err == p7r::ERR_NOT_IMPLEMENTED) {
    return std::shared_ptr<p7r::api>{};
  }
  return api;
}

} // anonymoys namespace

#define SKIP_IF_NOT_IMPLEMENTED \
  auto api = api_if_implemented(); \
  if (!api) { \
    GTEST_SKIP(); \
    return; \
  }


TEST(ExtConnectorTunTap, tun_create)
{
  SKIP_IF_NOT_IMPLEMENTED;

  ASSERT_NO_THROW(auto conn = p7r::connector(api, "tun:///tun_test"));
}


TEST(ExtConnectorTunTap, tap_create)
{
  SKIP_IF_NOT_IMPLEMENTED;

  ASSERT_NO_THROW(auto conn = p7r::connector(api, "tap:///tap_test"));
}


TEST(ExtConnectorTunTap, tun_listen)
{
  SKIP_IF_NOT_IMPLEMENTED;

  p7r::connector conn;
  ASSERT_NO_THROW(conn = p7r::connector(api, "tun:///tun_test"));

  ASSERT_FALSE(conn.listening());
  ASSERT_FALSE(conn.connected());

  // Listen or connect - it's the same on this connector

  auto err = conn.listen();
  ASSERT_EQ(p7r::ERR_SUCCESS, err);

  ASSERT_TRUE(conn.listening());
  ASSERT_TRUE(conn.connected());
}


TEST(ExtConnectorTunTap, tap_listen)
{
  SKIP_IF_NOT_IMPLEMENTED;

  p7r::connector conn;
  ASSERT_NO_THROW(conn = p7r::connector(api, "tap:///tap_test"));

  ASSERT_FALSE(conn.listening());
  ASSERT_FALSE(conn.connected());

  // Listen or connect - it's the same on this connector

  auto err = conn.listen();
  ASSERT_EQ(p7r::ERR_SUCCESS, err);

  ASSERT_TRUE(conn.listening());
  ASSERT_TRUE(conn.connected());
}


TEST(ExtConnectorTunTap, auto_select_name)
{
  SKIP_IF_NOT_IMPLEMENTED;

  p7r::connector conn;
  ASSERT_NO_THROW(conn = p7r::connector(api, "tun:///?mtu=200"));
  ASSERT_EQ("tun", conn.peer_addr().scheme());
  ASSERT_EQ(liberate::net::socket_address{"/"}, conn.peer_addr().socket_address());

  // Listen fills in the socket address with the actual device name being
  // used.
  auto err = conn.listen();
  ASSERT_EQ(p7r::ERR_SUCCESS, err);
  ASSERT_EQ("tun", conn.peer_addr().scheme());
  ASSERT_NE(liberate::net::socket_address{"/"}, conn.peer_addr().socket_address());
  auto url = conn.connect_url();
  ASSERT_GT(url.path.size(), 1);
  ASSERT_NE(url.query.end(), url.query.find("mtu"));
}
