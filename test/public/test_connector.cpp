/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2014 Unwesen Ltd.
 * Copyright (c) 2015-2020 Jens Finkhaeuser.
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

#include <packeteer/connector.h>
#include <packeteer/scheduler.h>

#include "../lib/macros.h"

#include <cstring>

#include <stdexcept>

#include <chrono>
#include <thread>

#include "../value_tests.h"
#include "../test_name.h"

#define TEST_SLEEP_TIME std::chrono::milliseconds(20)

using namespace packeteer;

/*****************************************************************************
 * ConnectorParsing
 */

namespace {

struct parsing_test_data
{
  char const *    address;
  bool            valid;
  connector_type  type;
} parsing_tests[] = {
  // Garbage
  { "foo", false, CT_UNSPEC },
  { "foo:", false, CT_UNSPEC },
  { "foo://", false, CT_UNSPEC },
  { "foo:///some/path", false, CT_UNSPEC },
  { "foo://123.123.133.123:12", false, CT_UNSPEC },
  { "tcp://foo", false, CT_UNSPEC },
  { "tcp4://foo", false, CT_UNSPEC },
  { "tcp6://foo", false, CT_UNSPEC },
  { "udp://foo", false, CT_UNSPEC },
  { "udp4://foo", false, CT_UNSPEC },
  { "udp6://foo", false, CT_UNSPEC },
  { "file://", false, CT_UNSPEC },
  { "ipc://", false, CT_UNSPEC },
  { "pipe://", false, CT_UNSPEC },
  { "anon://anything/here", false, CT_UNSPEC },

  // IPv4 hosts
  { "tcp://192.168.0.1",      true, CT_TCP },
  { "tcp://192.168.0.1:8080", true, CT_TCP },
  { "tCp://192.168.0.1",      true, CT_TCP },
  { "tcP://192.168.0.1:8080", true, CT_TCP },

  { "tcp4://192.168.0.1",      true, CT_TCP4 },
  { "tcp4://192.168.0.1:8080", true, CT_TCP4 },
  { "tCp4://192.168.0.1",      true, CT_TCP4 },
  { "tcP4://192.168.0.1:8080", true, CT_TCP4 },

  { "tcp4://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    false, CT_UNSPEC, },
  { "tcp4://2001:0db8:85a3:0:0:8a2e:0370:7334",          false, CT_UNSPEC, },
  { "tcp4://2001:0db8:85a3::8a2e:0370:7334",             false, CT_UNSPEC, },
  { "Tcp4://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    false, CT_UNSPEC, },
  { "tCp4://2001:0db8:85a3:0:0:8a2e:0370:7334",          false, CT_UNSPEC, },
  { "tcP4://2001:0db8:85a3::8a2e:0370:7334",             false, CT_UNSPEC, },

  { "udp://192.168.0.1",      true, CT_UDP },
  { "udp://192.168.0.1:8080", true, CT_UDP },
  { "uDp://192.168.0.1",      true, CT_UDP },
  { "udP://192.168.0.1:8080", true, CT_UDP },

  { "udp4://192.168.0.1",      true, CT_UDP4 },
  { "udp4://192.168.0.1:8080", true, CT_UDP4 },
  { "uDp4://192.168.0.1",      true, CT_UDP4 },
  { "udP4://192.168.0.1:8080", true, CT_UDP4 },

  { "udp4://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    false, CT_UNSPEC, },
  { "udp4://2001:0db8:85a3:0:0:8a2e:0370:7334",          false, CT_UNSPEC, },
  { "udp4://2001:0db8:85a3::8a2e:0370:7334",             false, CT_UNSPEC, },
  { "Udp4://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    false, CT_UNSPEC, },
  { "uDp4://2001:0db8:85a3:0:0:8a2e:0370:7334",          false, CT_UNSPEC, },
  { "udP4://2001:0db8:85a3::8a2e:0370:7334",             false, CT_UNSPEC, },

  // IPv6 hosts
  { "tcp://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true, CT_TCP, },
  { "tcp://2001:0db8:85a3:0:0:8a2e:0370:7334",          true, CT_TCP, },
  { "tcp://2001:0db8:85a3::8a2e:0370:7334",             true, CT_TCP, },
  { "Tcp://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true, CT_TCP, },
  { "tCp://2001:0db8:85a3:0:0:8a2e:0370:7334",          true, CT_TCP, },
  { "tcP://2001:0db8:85a3::8a2e:0370:7334",             true, CT_TCP, },

  { "tcp6://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true, CT_TCP6, },
  { "tcp6://2001:0db8:85a3:0:0:8a2e:0370:7334",          true, CT_TCP6, },
  { "tcp6://2001:0db8:85a3::8a2e:0370:7334",             true, CT_TCP6, },
  { "Tcp6://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true, CT_TCP6, },
  { "tCp6://2001:0db8:85a3:0:0:8a2e:0370:7334",          true, CT_TCP6, },
  { "tcP6://2001:0db8:85a3::8a2e:0370:7334",             true, CT_TCP6, },

  { "tcp6://192.168.0.1",      false, CT_UNSPEC },
  { "tcp6://192.168.0.1:8080", false, CT_UNSPEC },
  { "tCp6://192.168.0.1",      false, CT_UNSPEC },
  { "tcP6://192.168.0.1:8080", false, CT_UNSPEC },

  { "udp://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true, CT_UDP, },
  { "udp://2001:0db8:85a3:0:0:8a2e:0370:7334",          true, CT_UDP, },
  { "udp://2001:0db8:85a3::8a2e:0370:7334",             true, CT_UDP, },
  { "Udp://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true, CT_UDP, },
  { "uDp://2001:0db8:85a3:0:0:8a2e:0370:7334",          true, CT_UDP, },
  { "udP://2001:0db8:85a3::8a2e:0370:7334",             true, CT_UDP, },

  { "udp6://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true, CT_UDP6, },
  { "udp6://2001:0db8:85a3:0:0:8a2e:0370:7334",          true, CT_UDP6, },
  { "udp6://2001:0db8:85a3::8a2e:0370:7334",             true, CT_UDP6, },
  { "Udp6://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true, CT_UDP6, },
  { "uDp6://2001:0db8:85a3:0:0:8a2e:0370:7334",          true, CT_UDP6, },
  { "udP6://2001:0db8:85a3::8a2e:0370:7334",             true, CT_UDP6, },

  { "udp6://192.168.0.1",      false, CT_UNSPEC },
  { "udp6://192.168.0.1:8080", false, CT_UNSPEC },
  { "udP6://192.168.0.1",      false, CT_UNSPEC },
  { "uDp6://192.168.0.1:8080", false, CT_UNSPEC },

  // All other types require path names. There's not much common
  // about path names, so our only requirement is that it exists.
  { "local:///foo", true, CT_LOCAL },
  { "pipe:///foo", true, CT_PIPE },
  { "anon://", true, CT_ANON },

};

std::string connector_name(testing::TestParamInfo<parsing_test_data> const & info)
{
  return symbolize_name(info.param.address);
}

} // anonymous namespace


class ConnectorParsing
  : public testing::TestWithParam<parsing_test_data>
{
};


TEST_P(ConnectorParsing, parsing)
{
  using namespace packeteer;
  auto td = GetParam();

  // std::cout << "test: " << td.address << std::endl;

  if (td.valid) {
    ASSERT_NO_THROW(auto c = connector(test_env->api, td.address));
    auto c = connector(test_env->api, td.address);
    ASSERT_EQ(td.type, c.type());
  }
  else {
    ASSERT_THROW(auto c = connector(test_env->api, td.address), std::runtime_error);
  }
}


INSTANTIATE_TEST_CASE_P(net, ConnectorParsing,
    testing::ValuesIn(parsing_tests),
    connector_name);


/*****************************************************************************
 * Connector
 */


TEST(Connector, value_semantics)
{
  // We'll use an anon connector, because they're simplest.
  connector original{test_env->api, "anon://"};
  ASSERT_EQ(CT_ANON, original.type());
  ASSERT_TRUE(original);

  test_copy_construction(original);
  test_assignment(original);

  connector copy = original;
  ASSERT_EQ(original.type(), copy.type());
  ASSERT_EQ(original.connect_url(), copy.connect_url());
  ASSERT_EQ(original.get_read_handle(), copy.get_read_handle());
  ASSERT_EQ(original.get_write_handle(), copy.get_write_handle());

  test_equality(original, copy);

  // Hashing and swapping require different types
  connector different{test_env->api, "pipe:///foo"};
  test_hashing_inequality(original, different);
  test_hashing_equality(original, copy);
  test_swapping(original, different);
}


TEST(Connector, default_constructed)
{
  // Default constructed connectors should vaguely work.
  connector conn;
  ASSERT_EQ(CT_UNSPEC, conn.type());
  ASSERT_FALSE(conn);

  ASSERT_THROW(auto url = conn.connect_url(), exception);

  // Most functions should just return ERR_INITIALIZATION
  ASSERT_THROW(conn.is_blocking(), exception);
  ASSERT_THROW(conn.get_options(), exception);

  // Comparison should always yield the unspecified connector to be smaller.
  connector conn2;
  ASSERT_FALSE(conn2);
  ASSERT_EQ(conn, conn2);
  ASSERT_EQ(conn2, conn);

  // Either default-constructed connector should consider
  // itself smaller than the other.
  ASSERT_LT(conn, conn2);
  ASSERT_LT(conn2, conn);

  // Anonymous connectors are greater than default-constructed ones
  connector anon{test_env->api, "anon://"};
  ASSERT_TRUE(anon);
  ASSERT_LT(conn, anon);
  ASSERT_GT(anon, conn);

  // Assigning does work, though
  conn = anon;
  ASSERT_TRUE(conn);
  ASSERT_EQ(conn, anon);
  ASSERT_EQ(anon, conn);

  // Afterwards, conn (which is now anonymoys) should
  // evaluate as greater than conn2 (default)
  ASSERT_NE(conn, conn2);
  ASSERT_LT(conn2, conn);
  ASSERT_GT(conn, conn2);
}


/*****************************************************************************
 * ConnectorStream
 */
namespace {

struct streaming_test_data
{
  connector_type  type;
  char const *    stream_blocking;
  char const *    stream_non_blocking;
} streaming_tests[] = {
  { CT_LOCAL,
    "local:///tmp/test-connector-local-stream-block?blocking=1",
    "local:///tmp/test-connector-local-stream-noblock", },
  { CT_TCP4,
    "tcp4://127.0.0.1:54321?blocking=1",
    "tcp4://127.0.0.1:54321", },
  { CT_TCP6,
    "tcp6://[::1]:54321?blocking=1",
    "tcp6://[::1]:54321", },
  { CT_PIPE,
    "pipe:///tmp/test-connector-pipe-block?blocking=1",
    "pipe:///tmp/test-connector-pipe-noblock", },
};

template <typename T>
std::string connector_name(testing::TestParamInfo<T> const & info)
{
  switch (info.param.type) {
    case CT_TCP4:
      return "tcp4";

    case CT_TCP6:
      return "tcp6";

    case CT_UDP4:
      return "udp4";

    case CT_UDP6:
      return "udp6";

    case CT_LOCAL:
      return "local";

    case CT_PIPE:
      return "pipe";

      // TODO
    default:
      ADD_FAILURE_AT(__FILE__, __LINE__) << "Unreachable line reached.";
  }

  return {}; // silence compiler warnings
}


void peek_message_streaming(connector & sender, connector & receiver,
    int marker = -1)
{
  std::string msg = "Hello, world!";
  if (marker >= 0) {
    msg += " [" + std::to_string(marker) + "]";
  }
  size_t amount = 0;
  ASSERT_EQ(ERR_SUCCESS, sender.write(msg.c_str(), msg.size(), amount));
  ASSERT_EQ(msg.size(), amount);

  std::this_thread::sleep_for(TEST_SLEEP_TIME);

  auto peeked = receiver.peek();
  ASSERT_EQ(msg.size(), peeked);
}



void send_message_streaming(connector & sender, connector & receiver,
    int marker = -1, scheduler * sched = nullptr)
{
  std::string msg = "Hello, world!";
  if (marker >= 0) {
    msg += " [" + std::to_string(marker) + "]";
  }
  size_t amount = 0;
  ASSERT_EQ(ERR_SUCCESS, sender.write(msg.c_str(), msg.size(), amount));
  ASSERT_EQ(msg.size(), amount);

  if (sched) {
    sched->process_events(TEST_SLEEP_TIME);
  }
  else {
    std::this_thread::sleep_for(TEST_SLEEP_TIME);
  }

  std::vector<char> result;
  result.resize(2 * msg.size(), '\0');
  ASSERT_EQ(ERR_SUCCESS, receiver.read(&result[0], result.capacity(),
        amount));
  ASSERT_EQ(msg.size(), amount);
  result.resize(amount);

  std::string received{result.begin(), result.end()};
  DLOG("Sent '" << msg << "' and received '" << received << "'");
  ASSERT_EQ(msg, received);
}


struct server_connect_callback
{
  connector & m_server;
  connector   m_conn;

  server_connect_callback(connector & server)
    : m_server(server)
    , m_conn()
  {
  }

  ::packeteer::error_t
  func([[maybe_unused]] ::packeteer::events_t mask,
      [[maybe_unused]] ::packeteer::error_t error,
      [[maybe_unused]] connector const & conn, void *)
  {
    if (!m_conn) {
      DLOG(" ***** INCOMING " << mask << ":" << error << ":" << conn);
      // The accept() function clears the event.
      m_conn = m_server.accept();
      EXPECT_TRUE(m_conn);
    }
    return ERR_SUCCESS;
  }
};


struct client_post_connect_callback
{
  bool m_connected = false;

  ::packeteer::error_t
  func([[maybe_unused]] ::packeteer::events_t mask,
      [[maybe_unused]] ::packeteer::error_t error,
      [[maybe_unused]] connector const & conn, void *)
  {
    if (!m_connected) {
      m_connected = true;
      DLOG(" ***** CONNECTED! " << mask << ":" << error << ":" << conn);
    }

    return ERR_SUCCESS;
  }
};


} // anonymous namespace

class ConnectorStream
  : public testing::TestWithParam<streaming_test_data>
{
};


TEST_P(ConnectorStream, blocking_messaging)
{
  // Tests for "stream" connectors, i.e. connectors that allow synchronous,
  // reliable delivery - in blocking mode, making the setup/teardown very simple.
  auto td = GetParam();

  auto url = ::packeteer::util::url::parse(td.stream_blocking);
  url.query["behaviour"] = "stream";

  // Server
  connector server{test_env->api, url};
  ASSERT_EQ(td.type, server.type());

  ASSERT_FALSE(server.listening());
  ASSERT_FALSE(server.connected());

  ASSERT_EQ(ERR_SUCCESS, server.listen());

  ASSERT_TRUE(server.listening());
  ASSERT_FALSE(server.connected());

  ASSERT_TRUE(server.is_blocking());
  ASSERT_EQ(CO_STREAM|CO_BLOCKING, server.get_options());

  std::this_thread::sleep_for(TEST_SLEEP_TIME);

  // Client
  connector client{test_env->api, url};
  ASSERT_EQ(td.type, client.type());

  ASSERT_FALSE(client.listening());
  ASSERT_FALSE(client.connected());

  ASSERT_EQ(ERR_SUCCESS, client.connect());
  connector server_conn = server.accept();

  std::this_thread::sleep_for(TEST_SLEEP_TIME);

  ASSERT_FALSE(client.listening());
  ASSERT_TRUE(client.connected());
  ASSERT_TRUE(server_conn.listening());

  ASSERT_TRUE(server_conn.is_blocking());
  ASSERT_EQ(CO_STREAM|CO_BLOCKING, server_conn.get_options());

  ASSERT_TRUE(client.is_blocking());
  ASSERT_EQ(CO_STREAM|CO_BLOCKING, client.get_options());

  // Communications
  send_message_streaming(client, server_conn);
  send_message_streaming(server_conn, client);
}



TEST_P(ConnectorStream, non_blocking_messaging)
{
  // Tests for "stream" connectors, i.e. connectors that allow synchronous,
  // reliable delivery - in non-blocking mode, meaning we need to react to
  // events with the scheduler.
  auto td = GetParam();

  auto url = ::packeteer::util::url::parse(td.stream_non_blocking);
  url.query["behaviour"] = "stream";

  // Server
  connector server{test_env->api, url};
  ASSERT_EQ(td.type, server.type());

  ASSERT_FALSE(server.listening());
  ASSERT_FALSE(server.connected());

  ASSERT_EQ(ERR_SUCCESS, server.listen());

  ASSERT_TRUE(server.listening());
  ASSERT_FALSE(server.connected());

  ASSERT_FALSE(server.is_blocking());
  ASSERT_EQ(CO_STREAM|CO_NON_BLOCKING, server.get_options());

  std::this_thread::sleep_for(TEST_SLEEP_TIME);

  // Client
  connector client{test_env->api, url};
  ASSERT_EQ(td.type, client.type());

  ASSERT_FALSE(client.listening());
  ASSERT_FALSE(client.connected());

  // Connecting must result in ERR_ASYNC. We use a scheduler run to
  // understand when the connection attempt was finished.
  scheduler sched(test_env->api, 0);
  server_connect_callback server_struct(server);
  auto server_cb = make_callback(&server_struct, &server_connect_callback::func);
  sched.register_connector(PEV_IO_READ|PEV_IO_WRITE, server, server_cb);

  // Give scheduler a chance to register connectors
  sched.process_events(TEST_SLEEP_TIME);
  ASSERT_EQ(ERR_ASYNC, client.connect());

  client_post_connect_callback client_struct;
  auto client_cb = make_callback(&client_struct, &client_post_connect_callback::func);
  sched.register_connector(PEV_IO_READ|PEV_IO_WRITE, client, client_cb);

  // Wait for all callbacks to be invoked.
  sched.process_events(TEST_SLEEP_TIME);

  // After the sleep, the server conn and client conn should both
  // be ready to roll.
  ASSERT_TRUE(server_struct.m_conn);
  ASSERT_TRUE(client_struct.m_connected);

  connector server_conn = server_struct.m_conn;

  ASSERT_FALSE(client.listening());
  ASSERT_TRUE(client.connected());
  ASSERT_TRUE(server_conn.listening());

  ASSERT_FALSE(server_conn.is_blocking());
  ASSERT_EQ(CO_STREAM|CO_NON_BLOCKING, server_conn.get_options());

  ASSERT_FALSE(client.is_blocking());
  ASSERT_EQ(CO_STREAM|CO_NON_BLOCKING, client.get_options());

  // Communications
  send_message_streaming(client, server_conn);
  send_message_streaming(server_conn, client);
}



TEST_P(ConnectorStream, multiple_clients)
{
  // Test multiple clients connect to a single server, and can exchange
  // messages.
  auto td = GetParam();

  auto url = ::packeteer::util::url::parse(td.stream_blocking);
  url.query["behaviour"] = "stream";

  // Server
  connector server{test_env->api, url};
  EXPECT_EQ(td.type, server.type());

  EXPECT_FALSE(server.listening());
  EXPECT_FALSE(server.connected());

  EXPECT_EQ(ERR_SUCCESS, server.listen());

  EXPECT_TRUE(server.listening());
  EXPECT_FALSE(server.connected());

  EXPECT_TRUE(server.is_blocking());
  EXPECT_EQ(CO_STREAM|CO_BLOCKING, server.get_options());

  std::this_thread::sleep_for(TEST_SLEEP_TIME);

  // Client #1
  connector client1{test_env->api, url};
  EXPECT_EQ(td.type, client1.type());

  EXPECT_FALSE(client1.listening());
  EXPECT_FALSE(client1.connected());

  EXPECT_EQ(ERR_SUCCESS, client1.connect());
  connector server_conn1 = server.accept();

  std::this_thread::sleep_for(TEST_SLEEP_TIME);

  EXPECT_FALSE(client1.listening());
  EXPECT_TRUE(client1.connected());
  EXPECT_TRUE(server_conn1.listening());

  EXPECT_TRUE(server_conn1.is_blocking());
  EXPECT_EQ(CO_STREAM|CO_BLOCKING, server_conn1.get_options());

  EXPECT_TRUE(client1.is_blocking());
  EXPECT_EQ(CO_STREAM|CO_BLOCKING, client1.get_options());

  // Client #2
  connector client2{test_env->api, url};
  EXPECT_EQ(td.type, client2.type());

  EXPECT_FALSE(client2.listening());
  EXPECT_FALSE(client2.connected());

  EXPECT_EQ(ERR_SUCCESS, client2.connect());
  connector server_conn2 = server.accept();

  std::this_thread::sleep_for(TEST_SLEEP_TIME);

  EXPECT_FALSE(client2.listening());
  EXPECT_TRUE(client2.connected());
  EXPECT_TRUE(server_conn2.listening());

  EXPECT_TRUE(server_conn2.is_blocking());
  EXPECT_EQ(CO_STREAM|CO_BLOCKING, server_conn2.get_options());

  EXPECT_TRUE(client2.is_blocking());
  EXPECT_EQ(CO_STREAM|CO_BLOCKING, client2.get_options());

  // Communications with client #1
  send_message_streaming(client1, server_conn1, 1);
  send_message_streaming(server_conn1, client1, 2);

  // Communications with client #2
  send_message_streaming(client2, server_conn2, 3);
  send_message_streaming(server_conn2, client2, 4);
}



TEST_P(ConnectorStream, peek_from_write)
{
  // Peek using
  auto td = GetParam();

  auto url = ::packeteer::util::url::parse(td.stream_non_blocking);
  url.query["behaviour"] = "stream";

  // Server
  connector server{test_env->api, url};
  ASSERT_EQ(td.type, server.type());

  ASSERT_FALSE(server.listening());
  ASSERT_FALSE(server.connected());

  ASSERT_EQ(ERR_SUCCESS, server.listen());

  ASSERT_TRUE(server.listening());
  ASSERT_FALSE(server.connected());

  ASSERT_FALSE(server.is_blocking());
  ASSERT_EQ(CO_STREAM|CO_NON_BLOCKING, server.get_options());

  std::this_thread::sleep_for(TEST_SLEEP_TIME);

  // Client
  connector client{test_env->api, url};
  ASSERT_EQ(td.type, client.type());

  ASSERT_FALSE(client.listening());
  ASSERT_FALSE(client.connected());

  // Connecting must result in ERR_ASYNC. We use a scheduler run to
  // understand when the connection attempt was finished.
  // FIXME own run loop, no background threads.
  scheduler sched(test_env->api, 1);
  server_connect_callback server_struct(server);
  auto server_cb = make_callback(&server_struct, &server_connect_callback::func);
  sched.register_connector(PEV_IO_READ|PEV_IO_WRITE, server, server_cb);

  // Give scheduler a chance to register connectors
  std::this_thread::sleep_for(TEST_SLEEP_TIME);
  ASSERT_EQ(ERR_ASYNC, client.connect());

  client_post_connect_callback client_struct;
  auto client_cb = make_callback(&client_struct, &client_post_connect_callback::func);
  sched.register_connector(PEV_IO_READ|PEV_IO_WRITE, client, client_cb);

  // Wait for all callbacks to be invoked.
  std::this_thread::sleep_for(TEST_SLEEP_TIME);

  // After the sleep, the server conn and client conn should both
  // be ready to roll.
  ASSERT_TRUE(server_struct.m_conn);
  ASSERT_TRUE(client_struct.m_connected);

  connector server_conn = server_struct.m_conn;
  std::this_thread::sleep_for(TEST_SLEEP_TIME);

  ASSERT_FALSE(client.listening());
  ASSERT_TRUE(client.connected());
  ASSERT_TRUE(server_conn.listening());

  ASSERT_FALSE(server_conn.is_blocking());
  ASSERT_EQ(CO_STREAM|CO_NON_BLOCKING, server_conn.get_options());

  ASSERT_FALSE(client.is_blocking());
  ASSERT_EQ(CO_STREAM|CO_NON_BLOCKING, client.get_options());

  // Communications
  peek_message_streaming(server_conn, client);
  peek_message_streaming(client, server_conn);

}



INSTANTIATE_TEST_CASE_P(net, ConnectorStream,
    testing::ValuesIn(streaming_tests),
    connector_name<streaming_test_data>);


/*****************************************************************************
 * ConnectorDGram
 */
namespace {

struct dgram_test_data
{
  connector_type  type;
  char const *    dgram_first;
  char const *    dgram_second;
  char const *    dgram_third;
} dgram_tests[] = {
  { CT_LOCAL,
    "local:///tmp/test-connector-local-dgram-first",
    "local:///tmp/test-connector-local-dgram-second",
    "local:///tmp/test-connector-local-dgram-third", },
  { CT_UDP4,
    "udp4://127.0.0.1:54321",
    "udp4://127.0.0.1:54322",
    "udp4://127.0.0.1:54323", },
  { CT_UDP6,
    "udp6://[::1]:54321",
    "udp6://[::1]:54322",
    "udp6://[::1]:54323", },
};


void send_message_dgram(connector & sender, connector & receiver,
    int marker = -1)
{
  std::string msg = "hello, world!";
  if (marker >= 0) {
    msg += " [" + std::to_string(marker) + "]";
  }
  size_t amount = 0;
  ASSERT_EQ(ERR_SUCCESS, sender.send(msg.c_str(), msg.size(), amount,
      receiver.peer_addr()));
  ASSERT_EQ(msg.size(), amount);

  std::this_thread::sleep_for(TEST_SLEEP_TIME);

  std::vector<char> result;
  result.resize(2 * msg.size(), '\0');
  packeteer::peer_address sendaddr;
  ASSERT_EQ(ERR_SUCCESS, receiver.receive(&result[0], result.capacity(),
        amount, sendaddr));
  ASSERT_EQ(msg.size(), amount);
  ASSERT_EQ(sender.peer_addr(), sendaddr);
  result.resize(amount);

  std::string received{result.begin(), result.end()};
  DLOG("Sent '" << msg << "' and received '" << received << "'");
  ASSERT_EQ(msg, received);
}


} // anonymous namespace

class ConnectorDGram
  : public testing::TestWithParam<dgram_test_data>
{
};


TEST_P(ConnectorDGram, messaging)
{
  // Tests for "datagram" connectors, i.e. connectors that allow synchronous,
  // un-reliable delivery.
  auto td = GetParam();

  auto surl = ::packeteer::util::url::parse(td.dgram_first);
  surl.query["behaviour"] = "datagram";
  auto curl = ::packeteer::util::url::parse(td.dgram_second);
  curl.query["behaviour"] = "datagram";

  // Server
  connector server{test_env->api, surl};
  ASSERT_EQ(td.type, server.type());

  ASSERT_FALSE(server.listening());
  ASSERT_FALSE(server.connected());

  ASSERT_EQ(ERR_SUCCESS, server.listen());

  ASSERT_TRUE(server.listening());
  ASSERT_FALSE(server.connected());

  std::this_thread::sleep_for(TEST_SLEEP_TIME);

  // Client
  connector client{test_env->api, curl};
  ASSERT_EQ(td.type, client.type());

  ASSERT_FALSE(client.listening());
  ASSERT_FALSE(client.connected());

  ASSERT_EQ(ERR_SUCCESS, client.listen());

  ASSERT_TRUE(client.listening());
  ASSERT_FALSE(client.connected());

  std::this_thread::sleep_for(TEST_SLEEP_TIME);

  // Communications
  send_message_dgram(client, server);
  send_message_dgram(server, client);
}


TEST_P(ConnectorDGram, multiple_clients)
{
  // Test multiple clients connect to a single server, and can exchange
  // messages.
  auto td = GetParam();

  auto surl = ::packeteer::util::url::parse(td.dgram_first);
  surl.query["behaviour"] = "datagram";
  auto curl1 = ::packeteer::util::url::parse(td.dgram_second);
  curl1.query["behaviour"] = "datagram";
  auto curl2 = ::packeteer::util::url::parse(td.dgram_third);
  curl2.query["behaviour"] = "datagram";

  // Server
  connector server{test_env->api, surl};
  ASSERT_EQ(td.type, server.type());

  ASSERT_FALSE(server.listening());
  ASSERT_FALSE(server.connected());

  ASSERT_EQ(ERR_SUCCESS, server.listen());

  ASSERT_TRUE(server.listening());
  ASSERT_FALSE(server.connected());

  std::this_thread::sleep_for(TEST_SLEEP_TIME);

  // Client #1
  connector client1{test_env->api, curl1};
  ASSERT_EQ(td.type, client1.type());

  ASSERT_FALSE(client1.listening());
  ASSERT_FALSE(client1.connected());

  ASSERT_EQ(ERR_SUCCESS, client1.listen());

  ASSERT_TRUE(client1.listening());
  ASSERT_FALSE(client1.connected());

  // Client #2
  connector client2{test_env->api, curl2};
  ASSERT_EQ(td.type, client2.type());

  ASSERT_FALSE(client2.listening());
  ASSERT_FALSE(client2.connected());

  ASSERT_EQ(ERR_SUCCESS, client2.listen());

  ASSERT_TRUE(client2.listening());


  std::this_thread::sleep_for(TEST_SLEEP_TIME);

  // Communications #1 and #2
  send_message_dgram(client1, server, 1);
  send_message_dgram(server, client1, 2);

  send_message_dgram(client2, server, 3);
  send_message_dgram(server, client2, 4);
}



INSTANTIATE_TEST_CASE_P(net, ConnectorDGram,
    testing::ValuesIn(dgram_tests),
    connector_name<dgram_test_data>);


/*****************************************************************************
 * ConnectorMisc
 */

TEST(ConnectorMisc, anon_connector)
{
  // Anonymous pipes are special in that they need only one connector for
  // communications.
  connector conn{test_env->api, "anon://"};
  ASSERT_EQ(CT_ANON, conn.type());

  ASSERT_FALSE(conn.listening());
  ASSERT_FALSE(conn.connected());

  ASSERT_EQ(ERR_SUCCESS, conn.listen());

  ASSERT_TRUE(conn.listening());
  ASSERT_TRUE(conn.connected());

  std::string msg = "hello, world!";
  size_t amount = 0;
  ASSERT_EQ(ERR_SUCCESS, conn.write(msg.c_str(), msg.size(), amount));
  ASSERT_EQ(msg.size(), amount);

  std::vector<char> result;
  result.resize(2 * msg.size(), '\0');
  ASSERT_EQ(ERR_SUCCESS, conn.read(&result[0], result.capacity(),
        amount));
  ASSERT_EQ(msg.size(), amount);

  for (size_t i = 0 ; i < msg.size() ; ++i) {
    ASSERT_EQ(msg[i], result[i]);
  }
}

