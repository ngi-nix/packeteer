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
#include <packeteer/util/tmp.h>
#include <packeteer/util/path.h>

#include "../lib/macros.h"

#include <cstring>

#include <stdexcept>

#include <chrono>
#include <thread>

#include "../value_tests.h"
#include "../test_name.h"

#define TEST_SLEEP_TIME std::chrono::milliseconds(20)

namespace p7r = packeteer;

/*****************************************************************************
 * ConnectorParsing
 */

namespace {

struct parsing_test_data
{
  char const *        address;
  bool                valid;
  p7r::connector_type type;
} parsing_tests[] = {
  // Garbage
  { "foo", false, p7r::CT_UNSPEC },
  { "foo:", false, p7r::CT_UNSPEC },
  { "foo://", false, p7r::CT_UNSPEC },
  { "foo:///some/path", false, p7r::CT_UNSPEC },
  { "foo://123.123.133.123:12", false, p7r::CT_UNSPEC },
  { "tcp://foo", false, p7r::CT_UNSPEC },
  { "tcp4://foo", false, p7r::CT_UNSPEC },
  { "tcp6://foo", false, p7r::CT_UNSPEC },
  { "udp://foo", false, p7r::CT_UNSPEC },
  { "udp4://foo", false, p7r::CT_UNSPEC },
  { "udp6://foo", false, p7r::CT_UNSPEC },
  { "file://", false, p7r::CT_UNSPEC },
  { "ipc://", false, p7r::CT_UNSPEC },
  { "anon://anything/here", false, p7r::CT_UNSPEC },

#if defined(PACKETEER_WIN32)
  { "pipe://", false, p7r::CT_UNSPEC },
#endif

#if defined(PACKETEER_POSIX)
  { "fifo://", false, p7r::CT_UNSPEC },
#endif

  // IPv4 hosts
  { "tcp://192.168.0.1",      true, p7r::CT_TCP },
  { "tcp://192.168.0.1:8080", true, p7r::CT_TCP },
  { "tCp://192.168.0.1",      true, p7r::CT_TCP },
  { "tcP://192.168.0.1:8080", true, p7r::CT_TCP },

  { "tcp4://192.168.0.1",      true, p7r::CT_TCP4 },
  { "tcp4://192.168.0.1:8080", true, p7r::CT_TCP4 },
  { "tCp4://192.168.0.1",      true, p7r::CT_TCP4 },
  { "tcP4://192.168.0.1:8080", true, p7r::CT_TCP4 },

  { "tcp4://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    false, p7r::CT_UNSPEC, },
  { "tcp4://2001:0db8:85a3:0:0:8a2e:0370:7334",          false, p7r::CT_UNSPEC, },
  { "tcp4://2001:0db8:85a3::8a2e:0370:7334",             false, p7r::CT_UNSPEC, },
  { "Tcp4://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    false, p7r::CT_UNSPEC, },
  { "tCp4://2001:0db8:85a3:0:0:8a2e:0370:7334",          false, p7r::CT_UNSPEC, },
  { "tcP4://2001:0db8:85a3::8a2e:0370:7334",             false, p7r::CT_UNSPEC, },

  { "udp://192.168.0.1",      true, p7r::CT_UDP },
  { "udp://192.168.0.1:8080", true, p7r::CT_UDP },
  { "uDp://192.168.0.1",      true, p7r::CT_UDP },
  { "udP://192.168.0.1:8080", true, p7r::CT_UDP },

  { "udp4://192.168.0.1",      true, p7r::CT_UDP4 },
  { "udp4://192.168.0.1:8080", true, p7r::CT_UDP4 },
  { "uDp4://192.168.0.1",      true, p7r::CT_UDP4 },
  { "udP4://192.168.0.1:8080", true, p7r::CT_UDP4 },

  { "udp4://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    false, p7r::CT_UNSPEC, },
  { "udp4://2001:0db8:85a3:0:0:8a2e:0370:7334",          false, p7r::CT_UNSPEC, },
  { "udp4://2001:0db8:85a3::8a2e:0370:7334",             false, p7r::CT_UNSPEC, },
  { "Udp4://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    false, p7r::CT_UNSPEC, },
  { "uDp4://2001:0db8:85a3:0:0:8a2e:0370:7334",          false, p7r::CT_UNSPEC, },
  { "udP4://2001:0db8:85a3::8a2e:0370:7334",             false, p7r::CT_UNSPEC, },

  // IPv6 hosts
  { "tcp://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true, p7r::CT_TCP, },
  { "tcp://2001:0db8:85a3:0:0:8a2e:0370:7334",          true, p7r::CT_TCP, },
  { "tcp://2001:0db8:85a3::8a2e:0370:7334",             true, p7r::CT_TCP, },
  { "Tcp://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true, p7r::CT_TCP, },
  { "tCp://2001:0db8:85a3:0:0:8a2e:0370:7334",          true, p7r::CT_TCP, },
  { "tcP://2001:0db8:85a3::8a2e:0370:7334",             true, p7r::CT_TCP, },

  { "tcp6://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true, p7r::CT_TCP6, },
  { "tcp6://2001:0db8:85a3:0:0:8a2e:0370:7334",          true, p7r::CT_TCP6, },
  { "tcp6://2001:0db8:85a3::8a2e:0370:7334",             true, p7r::CT_TCP6, },
  { "Tcp6://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true, p7r::CT_TCP6, },
  { "tCp6://2001:0db8:85a3:0:0:8a2e:0370:7334",          true, p7r::CT_TCP6, },
  { "tcP6://2001:0db8:85a3::8a2e:0370:7334",             true, p7r::CT_TCP6, },

  { "tcp6://192.168.0.1",      false, p7r::CT_UNSPEC },
  { "tcp6://192.168.0.1:8080", false, p7r::CT_UNSPEC },
  { "tCp6://192.168.0.1",      false, p7r::CT_UNSPEC },
  { "tcP6://192.168.0.1:8080", false, p7r::CT_UNSPEC },

  { "udp://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true, p7r::CT_UDP, },
  { "udp://2001:0db8:85a3:0:0:8a2e:0370:7334",          true, p7r::CT_UDP, },
  { "udp://2001:0db8:85a3::8a2e:0370:7334",             true, p7r::CT_UDP, },
  { "Udp://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true, p7r::CT_UDP, },
  { "uDp://2001:0db8:85a3:0:0:8a2e:0370:7334",          true, p7r::CT_UDP, },
  { "udP://2001:0db8:85a3::8a2e:0370:7334",             true, p7r::CT_UDP, },

  { "udp6://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true, p7r::CT_UDP6, },
  { "udp6://2001:0db8:85a3:0:0:8a2e:0370:7334",          true, p7r::CT_UDP6, },
  { "udp6://2001:0db8:85a3::8a2e:0370:7334",             true, p7r::CT_UDP6, },
  { "Udp6://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true, p7r::CT_UDP6, },
  { "uDp6://2001:0db8:85a3:0:0:8a2e:0370:7334",          true, p7r::CT_UDP6, },
  { "udP6://2001:0db8:85a3::8a2e:0370:7334",             true, p7r::CT_UDP6, },

  { "udp6://192.168.0.1",      false, p7r::CT_UNSPEC },
  { "udp6://192.168.0.1:8080", false, p7r::CT_UNSPEC },
  { "udP6://192.168.0.1",      false, p7r::CT_UNSPEC },
  { "uDp6://192.168.0.1:8080", false, p7r::CT_UNSPEC },

  // All other types require path names. There's not much common
  // about path names, so our only requirement is that it exists.
  { "local:///foo", true, p7r::CT_LOCAL },
  { "anon://", true, p7r::CT_ANON },

#if defined(PACKETEER_WIN32)
  { "pipe:///foo", true, p7r::CT_PIPE },
#endif

#if defined(PACKETEER_POSIX)
  { "fifo:///foo", true, p7r::CT_FIFO },
#endif
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
  auto td = GetParam();

  // std::cout << "test: " << td.address << std::endl;

  if (td.valid) {
    ASSERT_NO_THROW(auto c = p7r::connector(test_env->api, td.address));
    auto c = p7r::connector(test_env->api, td.address);
    ASSERT_EQ(td.type, c.type());
  }
  else {
    ASSERT_THROW(auto c = p7r::connector(test_env->api, td.address), std::runtime_error);
  }
}


INSTANTIATE_TEST_SUITE_P(net, ConnectorParsing,
    testing::ValuesIn(parsing_tests),
    connector_name);


/*****************************************************************************
 * Connector
 */


TEST(Connector, value_semantics)
{
  // We'll use an anon connector, because they're simplest.
  p7r::connector original{test_env->api, "anon://"};
  ASSERT_EQ(p7r::CT_ANON, original.type());
  ASSERT_TRUE(original);

  test_copy_construction(original);
  test_assignment(original);

  p7r::connector copy = original;
  ASSERT_EQ(original.type(), copy.type());
  ASSERT_EQ(original.connect_url(), copy.connect_url());
  ASSERT_EQ(original.get_read_handle(), copy.get_read_handle());
  ASSERT_EQ(original.get_write_handle(), copy.get_write_handle());

  test_equality(original, copy);

  // Hashing and swapping require different types
  p7r::connector different{test_env->api, "tcp://127.0.0.1"};
  test_hashing_inequality(original, different);
  test_hashing_equality(original, copy);
  test_swapping(original, different);
}


TEST(Connector, default_constructed)
{
  // Default constructed connectors should vaguely work.
  p7r::connector conn;
  ASSERT_EQ(p7r::CT_UNSPEC, conn.type());
  ASSERT_FALSE(conn);

  ASSERT_THROW(auto url = conn.connect_url(), p7r::exception);

  // Most functions should just return ERR_INITIALIZATION
  ASSERT_THROW(conn.is_blocking(), p7r::exception);
  ASSERT_THROW(conn.get_options(), p7r::exception);

  // Comparison should always yield the unspecified connector to be smaller.
  p7r::connector conn2;
  ASSERT_FALSE(conn2);
  ASSERT_EQ(conn, conn2);
  ASSERT_EQ(conn2, conn);

  // Neither default-constructed connector should consider
  // itself smaller than the other.
  ASSERT_LE(conn, conn2);
  ASSERT_LE(conn2, conn);
  ASSERT_GE(conn, conn2);
  ASSERT_GE(conn2, conn);

  // Anonymous connectors are greater than default-constructed ones
  p7r::connector anon{test_env->api, "anon://"};
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

namespace pu = packeteer::util;

inline std::string
name_with(std::string const & base, bool blocking)
{
  std::string name = base;
  if (blocking) {
    name += "-block";
  }
  else {
    name += "-noblock";
  }

  name = pu::to_posix_path(pu::temp_name(name));

  if (blocking) {
    name += "?blocking=1";
  }

  return name;
}



struct streaming_test_data
{
  p7r::connector_type               type;
  std::function<std::string (bool)> generator;
  bool                              broadcast = false;
} streaming_tests[] = {
  { p7r::CT_LOCAL,
    [](bool blocking) -> std::string
    {
      return "local://" + name_with("test-connector-local", blocking);
    },
  },
  { p7r::CT_TCP4,
    [](bool blocking) -> std::string
    {
      std::string ret = "tcp4://127.0.0.1:" + std::to_string(54321 + (rand() % 100));
      if (blocking) {
        ret += "?blocking=1";
      }
      return ret;
    },
  },
  { p7r::CT_TCP6,
    [](bool blocking) -> std::string
    {
      std::string ret = "tcp6://[::1]:" + std::to_string(54321 + (rand() % 100));
      if (blocking) {
        ret += "?blocking=1";
      }
      return ret;
    },
  },
#if defined(PACKETEER_WIN32)
  { p7r::CT_PIPE,
    [](bool blocking) -> std::string
    {
      return "pipe://" + name_with("test-connector-pipe", blocking);
    },
  },
#endif

#if defined(PACKETEER_POSIX)
  { p7r::CT_FIFO,
    [](bool blocking) -> std::string
    {
      return "fifo://" + name_with("test-connector-fifo", blocking);
    },
    // No need to convert from POSIX to POSIX
    true,
  },
#endif
};

template <typename T>
std::string connector_name(testing::TestParamInfo<T> const & info)
{
  switch (info.param.type) {
    case p7r::CT_TCP4:
      return "tcp4";

    case p7r::CT_TCP6:
      return "tcp6";

    case p7r::CT_UDP4:
      return "udp4";

    case p7r::CT_UDP6:
      return "udp6";

    case p7r::CT_LOCAL:
      return "local";

    case p7r::CT_PIPE:
      return "pipe";

    case p7r::CT_FIFO:
      return "fifo";

      // TODO
    default:
      ADD_FAILURE_AT(__FILE__, __LINE__) << "Unreachable line reached.";
  }

  return {}; // silence compiler warnings
}



void peek_message_streaming(p7r::connector & sender, p7r::connector & receiver,
    int marker = -1, p7r::scheduler * sched = nullptr)
{
  std::string msg = "Hello, world!";
  if (marker >= 0) {
    msg += " [" + std::to_string(marker) + "]";
  }
  size_t amount = 0;
  ASSERT_EQ(p7r::ERR_SUCCESS, sender.write(msg.c_str(), msg.size(), amount));
  ASSERT_EQ(msg.size(), amount);

  if (sched) {
    sched->process_events(TEST_SLEEP_TIME);
  }
  else {
    std::this_thread::sleep_for(TEST_SLEEP_TIME);
  }

  auto peeked = receiver.peek();

  // Depending on the OS and connector type, there may be a lot more
  // returned by peek() than the message size.
  ASSERT_GE(peeked, msg.size());
}



void send_message_streaming(p7r::connector & sender, p7r::connector & receiver,
    int marker = -1, p7r::scheduler * sched = nullptr)
{
  std::string msg = "Hello, world!";
  if (marker >= 0) {
    msg += " [" + std::to_string(marker) + "]";
  }
  size_t amount = 0;
  ASSERT_EQ(p7r::ERR_SUCCESS, sender.write(msg.c_str(), msg.size(), amount));
  ASSERT_EQ(msg.size(), amount);

  if (sched) {
    sched->process_events(TEST_SLEEP_TIME);
  }
  else {
    std::this_thread::sleep_for(TEST_SLEEP_TIME);
  }

  std::vector<char> result;
  result.resize(2 * msg.size(), '\0');
  ASSERT_EQ(p7r::ERR_SUCCESS, receiver.read(&result[0], result.capacity(),
        amount));
  ASSERT_EQ(msg.size(), amount);
  result.resize(amount);

  std::string received{result.begin(), result.end()};
  DLOG("Sent '" << msg << "' and received '" << received << "'");
  ASSERT_EQ(msg, received);
}



void send_message_streaming_async(p7r::connector & sender, p7r::connector & receiver,
    p7r::scheduler & sched, int marker = -1)
{
  // Register a read callback with the scheduler for the receiver connector.
  std::vector<char> result;
  auto lambda =
   [&result](p7r::time_point const & now [[maybe_unused]],
      p7r::events_t mask [[maybe_unused]],
      p7r::error_t error [[maybe_unused]],
      p7r::connector * conn, void *) -> p7r::error_t
  {
    EXPECT_EQ(mask, p7r::PEV_IO_READ);
    EXPECT_NE(conn, nullptr);
    size_t amount = 0;
    auto err = conn->read(&result[0], result.capacity(), amount);
    if (err == p7r::ERR_SUCCESS) {
      EXPECT_GT(amount, 0);
      result.resize(amount);
    }

    return p7r::ERR_SUCCESS;
  };

  sched.register_connector(p7r::PEV_IO_READ, receiver, lambda);
  sched.process_events(TEST_SLEEP_TIME);

  std::string msg = "Hello, world!";
  if (marker >= 0) {
    msg += " [" + std::to_string(marker) + "]";
  }
  result.resize(2 * msg.size(), '\0');
  size_t amount = 0;
  ASSERT_EQ(p7r::ERR_SUCCESS, sender.write(msg.c_str(), msg.size(), amount));
  ASSERT_EQ(msg.size(), amount);

  sched.process_events(TEST_SLEEP_TIME);

  sched.unregister_connector(p7r::PEV_IO_READ, receiver, lambda);
  sched.process_events(TEST_SLEEP_TIME);


  // After this, we should have had the read callback invoked, and the message
  // and result sizes should be the same.
  ASSERT_EQ(msg.size(), result.size());

  std::string received{result.begin(), result.end()};
  DLOG("Sent '" << msg << "' and received '" << received << "'");
  ASSERT_EQ(msg, received);
}


void setup_message_streaming_async(int index,
    std::vector<std::string> & expected,
    std::vector<std::string> & result,
    p7r::connector & receiver,
    p7r::scheduler & sched,
    bool broadcast
)
{
  // Create & register a message
  std::string msg = "Hello, world! [" + std::to_string(index) + "]";
  expected[index] = msg;
  // std::cout << "Expect " << msg << " on " << receiver << std::endl;

  // Register a read callback with the scheduler for the receiver connector.
  auto lambda = [msg, index, broadcast, &result](
      p7r::time_point const & now [[maybe_unused]],
      p7r::events_t mask [[maybe_unused]],
      p7r::error_t error [[maybe_unused]],
      p7r::connector * conn, void *) -> p7r::error_t
  {
    EXPECT_EQ(mask, p7r::PEV_IO_READ);
    EXPECT_NE(conn, nullptr);

    // *Drain* the connector
    p7r::error_t err = p7r::ERR_SUCCESS;
    do {
      std::vector<char> res;
      res.resize(msg.size());
      EXPECT_GT(res.capacity(), 0);

      size_t amount = 0;
      err = conn->read(&res[0], res.capacity(), amount);

      if (err == p7r::ERR_SUCCESS) {
        EXPECT_EQ(msg.size(), amount);

        res.resize(amount);
        std::string res_str{res.begin(), res.end()};
        // std::cout << "Got " << res_str << " on " << *conn << " (" << broadcast << ")" << std::endl;

        if (!broadcast) {
          EXPECT_EQ(msg, res_str);
        }
        else {
          result[index] += res_str;
        }
      }
    } while (err == p7r::ERR_SUCCESS);

    return p7r::ERR_SUCCESS;
  };

  sched.register_connector(p7r::PEV_IO_READ, receiver, lambda);
}



struct server_connect_callback
{
  p7r::connector & m_server;
  p7r::connector   m_conn;

  server_connect_callback(p7r::connector & server)
    : m_server(server)
    , m_conn()
  {
  }

  p7r::error_t
  func(p7r::time_point const & now [[maybe_unused]],
      p7r::events_t mask [[maybe_unused]],
      p7r::error_t error [[maybe_unused]],
      p7r::connector * conn [[maybe_unused]], void *)
  {
    if (!m_conn) {
      DLOG(" ***** INCOMING " << mask << ":" << error << ":" << *conn);
      // The accept() function clears the event.
      EXPECT_NO_THROW(m_conn = m_server.accept());
      EXPECT_TRUE(m_conn);
    }
    return p7r::ERR_SUCCESS;
  }
};


struct client_post_connect_callback
{
  bool m_connected = false;

  p7r::error_t
  func(p7r::time_point const & now [[maybe_unused]],
      p7r::events_t mask [[maybe_unused]],
      p7r::error_t error [[maybe_unused]],
      p7r::connector * conn [[maybe_unused]], void *)
  {
    if (!m_connected) {
      m_connected = true;
      DLOG(" ***** CONNECTED! " << mask << ":" << error << ":" << *conn);
    }

    return p7r::ERR_SUCCESS;
  }
};


std::vector<
  std::pair<p7r::connector, p7r::connector>
>
setup_stream_connection_async(p7r::scheduler & sched, p7r::connector_type type,
    p7r::util::url const & url, int num_clients = 1)
{
  // Server
  p7r::connector server{test_env->api, url};
  EXPECT_EQ(type, server.type());

  EXPECT_FALSE(server.listening());
  EXPECT_FALSE(server.connected());
  EXPECT_FALSE(server.communicating());

  EXPECT_EQ(p7r::ERR_SUCCESS, server.listen());

  EXPECT_TRUE(server.listening());
  EXPECT_FALSE(server.connected());
  EXPECT_FALSE(server.communicating());

  EXPECT_FALSE(server.is_blocking());
  EXPECT_EQ(p7r::CO_STREAM|p7r::CO_NON_BLOCKING, server.get_options());

  std::this_thread::sleep_for(TEST_SLEEP_TIME);

  std::vector<
    std::pair<p7r::connector, p7r::connector>
  > result;
  for (int i = 0 ; i < num_clients ; ++i) {
    // Client
    p7r::connector client{test_env->api, url};
    EXPECT_EQ(type, client.type());

    EXPECT_FALSE(client.listening());
    EXPECT_FALSE(client.connected());
    EXPECT_FALSE(client.communicating());

    // Connecting must result in p7r::ERR_ASYNC. We use a scheduler run to
    // understand when the connection attempt was finished.
    server_connect_callback server_struct(server);
    p7r::callback server_cb{&server_struct, &server_connect_callback::func};
    sched.register_connector(p7r::PEV_IO_READ|p7r::PEV_IO_WRITE, server, server_cb);

    // Give scheduler a chance to register connectors
    sched.process_events(TEST_SLEEP_TIME);
    auto connect_res = client.connect();
    EXPECT_EQ(p7r::ERR_ASYNC, connect_res);

    client_post_connect_callback client_struct;
    p7r::callback client_cb{&client_struct, &client_post_connect_callback::func};
    sched.register_connector(p7r::PEV_IO_READ|p7r::PEV_IO_WRITE, client, client_cb);

    // Wait for all callbacks to be invoked.
    sched.process_events(TEST_SLEEP_TIME);

    // After the sleep, the server conn and client conn should both
    // be ready to roll.
    EXPECT_TRUE(server_struct.m_conn);
    EXPECT_TRUE(client_struct.m_connected);

    p7r::connector server_conn = server_struct.m_conn;

    EXPECT_FALSE(client.listening());
    EXPECT_TRUE(client.connected());
    EXPECT_TRUE(client.communicating());

    EXPECT_TRUE(server_conn.listening());
    EXPECT_TRUE(server_conn.connected());
    EXPECT_TRUE(server_conn.communicating());

    EXPECT_TRUE(server.listening());
    EXPECT_FALSE(server.connected());
    EXPECT_FALSE(server.connected());

    EXPECT_FALSE(server_conn.is_blocking());
    EXPECT_EQ(p7r::CO_STREAM|p7r::CO_NON_BLOCKING, server_conn.get_options());

    EXPECT_FALSE(client.is_blocking());
    EXPECT_EQ(p7r::CO_STREAM|p7r::CO_NON_BLOCKING, client.get_options());

    // We're done with these local connectors
    sched.unregister_connector(p7r::PEV_IO_READ|p7r::PEV_IO_WRITE, server, server_cb);
    sched.unregister_connector(p7r::PEV_IO_READ|p7r::PEV_IO_WRITE, client, client_cb);

    result.push_back(std::make_pair(client, server_conn));
  }

  return result;
}




std::vector<
  std::pair<p7r::connector, p7r::connector>
>
setup_stream_connection(p7r::connector_type type, p7r::util::url const & url,
    int num_clients = 1)
{
  // Server
  p7r::connector server{test_env->api, url};
  EXPECT_EQ(type, server.type());

  EXPECT_FALSE(server.listening());
  EXPECT_FALSE(server.connected());
  EXPECT_FALSE(server.communicating());

  EXPECT_EQ(p7r::ERR_SUCCESS, server.listen());

  EXPECT_TRUE(server.listening());
  EXPECT_FALSE(server.connected());
  EXPECT_FALSE(server.communicating());

  EXPECT_TRUE(server.is_blocking());
  EXPECT_EQ(p7r::CO_STREAM|p7r::CO_BLOCKING, server.get_options());

  std::this_thread::sleep_for(TEST_SLEEP_TIME);

  // Clients
  std::vector<
    std::pair<p7r::connector, p7r::connector>
  > result;

  for (int i = 0 ; i < num_clients ; ++i) {
    p7r::connector client{test_env->api, url};
    EXPECT_EQ(type, client.type());

    EXPECT_FALSE(client.listening());
    EXPECT_FALSE(client.connected());
    EXPECT_FALSE(client.communicating());

    EXPECT_EQ(p7r::ERR_SUCCESS, client.connect());
    p7r::connector server_conn = server.accept();

    std::this_thread::sleep_for(TEST_SLEEP_TIME);

    EXPECT_FALSE(client.listening());
    EXPECT_TRUE(client.connected());
    EXPECT_TRUE(client.communicating());

    EXPECT_TRUE(server_conn.listening());
    EXPECT_TRUE(server_conn.connected());
    EXPECT_TRUE(server_conn.communicating());

    EXPECT_TRUE(server.listening());
    EXPECT_FALSE(server.connected());
    EXPECT_FALSE(server.connected());

    EXPECT_TRUE(server_conn.is_blocking());
    EXPECT_EQ(p7r::CO_STREAM|p7r::CO_BLOCKING, server_conn.get_options());

    EXPECT_TRUE(client.is_blocking());
    EXPECT_EQ(p7r::CO_STREAM|p7r::CO_BLOCKING, client.get_options());

    result.push_back(std::make_pair(client, server_conn));
  }

  return result;
}





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

  auto url = p7r::util::url::parse(td.generator(true));
  url.query["behaviour"] = "stream";
  std::cout << "URL: " << url << std::endl;

  auto res = setup_stream_connection(td.type, url);

  auto client = res[0].first;
  auto server = res[0].second;

  // Communications
  send_message_streaming(client, server);
  send_message_streaming(server, client);
}



TEST_P(ConnectorStream, non_blocking_messaging)
{
  // Tests for "stream" connectors, i.e. connectors that allow synchronous,
  // reliable delivery - in non-blocking mode, meaning we need to react to
  // events with the scheduler.
  auto td = GetParam();

  auto url = p7r::util::url::parse(td.generator(false));
  url.query["behaviour"] = "stream";

  p7r::scheduler sched(test_env->api, 0);
  auto res = setup_stream_connection_async(sched, td.type, url);

  auto client = res[0].first;
  auto server = res[0].second;

  // Communications
  send_message_streaming(client, server, -1, &sched);
  send_message_streaming(server, client, -1, &sched);
}



TEST_P(ConnectorStream, asynchronous_messaging)
{
  // After the same setup as in non_blocking_messaging, also perform the
  // messaging asynchronously.
  auto td = GetParam();

  auto url = p7r::util::url::parse(td.generator(false));
  url.query["behaviour"] = "stream";

  p7r::scheduler sched(test_env->api, 0);
  auto res = setup_stream_connection_async(sched, td.type, url);

  auto client = res[0].first;
  auto server = res[0].second;

  // Communications
  send_message_streaming_async(client, server, sched, 1);
  send_message_streaming_async(server, client, sched, 2);
}



TEST_P(ConnectorStream, multiple_clients_blocking)
{
  // Test multiple clients connect to a single server, and can exchange
  // messages.
  auto td = GetParam();

  auto url = p7r::util::url::parse(td.generator(true));
  url.query["behaviour"] = "stream";

  auto res = setup_stream_connection(td.type, url, 2);

  auto client1 = res[0].first;
  auto server1 = res[0].second;

  auto client2 = res[1].first;
  auto server2 = res[1].second;

  // Communications with client #1
  send_message_streaming(client1, server1, 1);
  send_message_streaming(server1, client1, 2);

  // Communications with client #2
  send_message_streaming(client2, server2, 3);
  send_message_streaming(server2, client2, 4);
}



TEST_P(ConnectorStream, multiple_clients_async)
{
  // Test multiple clients connect to a single server, and can exchange
  // messages.
  auto td = GetParam();

  auto url = p7r::util::url::parse(td.generator(false));
  url.query["behaviour"] = "stream";

  p7r::scheduler sched(test_env->api, 0);
  auto res = setup_stream_connection_async(sched, td.type, url, 2);

  auto client1 = res[0].first;
  auto server1 = res[0].second;

  auto client2 = res[1].first;
  auto server2 = res[1].second;

  // Messaging setup
  std::vector<std::string> expected;
  expected.resize(4);
  std::vector<std::string> result;
  result.resize(4);

  setup_message_streaming_async(0, expected, result, client1, sched, td.broadcast);
  setup_message_streaming_async(1, expected, result, server1, sched, td.broadcast);
  setup_message_streaming_async(2, expected, result, client2, sched, td.broadcast);
  setup_message_streaming_async(3, expected, result, server2, sched, td.broadcast);

  // Process events for registering callbacks
  sched.process_events(TEST_SLEEP_TIME);

  // Now send the messages.
  size_t amount = 0;
  ASSERT_EQ(p7r::ERR_SUCCESS, server1.write(expected[0].c_str(), expected[0].size(),
        amount));
  ASSERT_EQ(expected[0].size(), amount);

  amount = 0;
  ASSERT_EQ(p7r::ERR_SUCCESS, client1.write(expected[1].c_str(), expected[1].size(),
        amount));
  ASSERT_EQ(expected[1].size(), amount);

  amount = 0;
  ASSERT_EQ(p7r::ERR_SUCCESS, server2.write(expected[2].c_str(), expected[2].size(),
        amount));
  ASSERT_EQ(expected[2].size(), amount);

  amount = 0;
  ASSERT_EQ(p7r::ERR_SUCCESS, client2.write(expected[3].c_str(), expected[3].size(),
        amount));
  ASSERT_EQ(expected[3].size(), amount);

  // Process I/O
  sched.process_events(TEST_SLEEP_TIME * 2);

  // There isn't really anything else to do now; the callbacks contain the
  // actual tests. Except, when the connector is broadcasting (FIFO), then
  // we need to check the result matches the expectations loosely.
  if (td.broadcast) {
    // We can't predict which of the connectors picks up how many of the messages,
    // or in what order. It could be all on one, or spread out. So all we can
    // do is concatenate all results, and search for our expected messages in
    // that - all expected need to be found.
    std::string concat;
    for (auto & p : result) {
      concat += p;
    }

    // Test
    for (auto & exp : expected) {
      ASSERT_NE(std::string::npos, concat.find(exp));
    }
  }
}




TEST_P(ConnectorStream, peek_from_write)
{
  // Peek using
  auto td = GetParam();

  auto url = p7r::util::url::parse(td.generator(false));
  url.query["behaviour"] = "stream";

  p7r::scheduler sched(test_env->api, 0);
  auto res = setup_stream_connection_async(sched, td.type, url);

  auto client = res[0].first;
  auto server = res[0].second;

  // Communications
  peek_message_streaming(server, client, -1, &sched);
  peek_message_streaming(client, server, -1, &sched);
}



INSTANTIATE_TEST_SUITE_P(net, ConnectorStream,
    testing::ValuesIn(streaming_tests),
    connector_name<streaming_test_data>);


/*****************************************************************************
 * ConnectorDGram
 */
namespace {

struct dgram_test_data
{
  p7r::connector_type type;
  char const *        dgram_first;
  char const *        dgram_second;
  char const *        dgram_third;
} dgram_tests[] = {
  { p7r::CT_LOCAL,
    "local:///tmp/test-connector-local-dgram-first?blocking=1",
    "local:///tmp/test-connector-local-dgram-second?blocking=1",
    "local:///tmp/test-connector-local-dgram-third?blocking=1", },
  { p7r::CT_UDP4,
    "udp4://127.0.0.1:54321?blocking=1",
    "udp4://127.0.0.1:54322?blocking=1",
    "udp4://127.0.0.1:54323?blocking=1", },
  { p7r::CT_UDP6,
    "udp6://[::1]:54321?blocking=1",
    "udp6://[::1]:54322?blocking=1",
    "udp6://[::1]:54323?blocking=1", },
};


void send_message_dgram(p7r::connector & sender, p7r::connector & receiver,
    int marker = -1)
{
  std::string msg = "hello, world!";
  if (marker >= 0) {
    msg += " [" + std::to_string(marker) + "]";
  }
  size_t amount = 0;
  ASSERT_EQ(p7r::ERR_SUCCESS, sender.send(msg.c_str(), msg.size(), amount,
      receiver.peer_addr()));
  ASSERT_EQ(msg.size(), amount);

  std::this_thread::sleep_for(TEST_SLEEP_TIME);

  std::vector<char> result;
  result.resize(2 * msg.size(), '\0');
  packeteer::peer_address sendaddr;
  ASSERT_EQ(p7r::ERR_SUCCESS, receiver.receive(&result[0], result.capacity(),
        amount, sendaddr));
  ASSERT_EQ(msg.size(), amount);
  ASSERT_EQ(sender.peer_addr(), sendaddr);
  result.resize(amount);

  std::string received{result.begin(), result.end()};
  DLOG("Sent '" << msg << "' and received '" << received << "'");
  ASSERT_EQ(msg, received);
}



void send_message_dgram_async(int index,
    std::multimap<p7r::peer_address, std::string> & result,
    p7r::connector & sender,
    p7r::connector & receiver,
    p7r::scheduler & sched)
{
  // Create a message
  std::string msg = "Hello, world! [" + std::to_string(index) + "]";

  // Register a read callback with the scheduler
  auto lambda = [sender, &result](
      p7r::time_point const & now [[maybe_unused]],
      p7r::events_t mask [[maybe_unused]],
      p7r::error_t error [[maybe_unused]],
      p7r::connector * conn, void *) -> p7r::error_t
  {
    EXPECT_EQ(mask, p7r::PEV_IO_READ);
    EXPECT_NE(conn, nullptr);

    p7r::error_t err = p7r::ERR_SUCCESS;
    do {
      std::vector<char> res;
      res.resize(100); // Some amount > msg.size()
      EXPECT_GT(res.capacity(), 0);

      p7r::peer_address peer;
      size_t amount = 0;
      err = conn->receive(&res[0], res.capacity(), amount, peer);

      if (err == p7r::ERR_SUCCESS) {
        EXPECT_GE(amount, 0);

        res.resize(amount);
        std::string res_str{res.begin(), res.end()};
        // std::cout << "Got " << res_str << " from " << peer << std::endl;

        result.insert(std::make_pair(peer, res_str));
      }
    } while (err == p7r::ERR_SUCCESS);

    return p7r::ERR_SUCCESS;
  };

  sched.register_connector(p7r::PEV_IO_READ, receiver, lambda);

  // We can send immediately.
  size_t bytes_written = 0;
  auto err = sender.send(msg.c_str(), msg.size(), bytes_written, receiver.peer_addr());
  EXPECT_EQ(p7r::ERR_SUCCESS, err);
  EXPECT_EQ(msg.size(), bytes_written);
}




void peek_message_dgram(p7r::connector & sender, p7r::connector & receiver,
    int marker = -1)
{
  std::string msg = "hello, world!";
  if (marker >= 0) {
    msg += " [" + std::to_string(marker) + "]";
  }
  size_t amount = 0;
  ASSERT_EQ(p7r::ERR_SUCCESS, sender.send(msg.c_str(), msg.size(), amount,
      receiver.peer_addr()));
  ASSERT_EQ(msg.size(), amount);

  std::this_thread::sleep_for(TEST_SLEEP_TIME);

  auto peeked = receiver.peek();

  // Depending on the OS and connector type, there may be a lot more
  // returned by peek() than the message size.
  ASSERT_GE(peeked, msg.size());
}



std::pair<
  p7r::connector,
  std::vector<p7r::connector>
>
setup_dgram_connection(p7r::connector_type type, p7r::util::url const & server_url,
    std::vector<p7r::util::url> const & client_urls)
{
  // Server
  p7r::connector server{test_env->api, server_url};
  EXPECT_EQ(type, server.type());

  EXPECT_FALSE(server.listening());
  EXPECT_FALSE(server.connected());
  EXPECT_FALSE(server.communicating());

  EXPECT_EQ(p7r::ERR_SUCCESS, server.listen());

  EXPECT_TRUE(server.listening());
  EXPECT_FALSE(server.connected());
  EXPECT_TRUE(server.communicating());

  EXPECT_TRUE(server.is_blocking());
  EXPECT_EQ(p7r::CO_DATAGRAM|p7r::CO_BLOCKING, server.get_options());

  std::this_thread::sleep_for(TEST_SLEEP_TIME);

  std::vector<p7r::connector> result;
  for (size_t i = 0 ; i < client_urls.size() ; ++i) {
    // Client
    p7r::connector client{test_env->api, client_urls[i]};
    EXPECT_EQ(type, client.type());

    EXPECT_FALSE(client.listening());
    EXPECT_FALSE(client.connected());
    EXPECT_FALSE(client.communicating());

    EXPECT_EQ(p7r::ERR_SUCCESS, client.listen());

    EXPECT_TRUE(client.listening());
    EXPECT_FALSE(client.connected());
    EXPECT_TRUE(client.communicating());

    EXPECT_TRUE(client.is_blocking());
    EXPECT_EQ(p7r::CO_DATAGRAM|p7r::CO_BLOCKING, client.get_options());

    result.push_back(client);
  }

  std::this_thread::sleep_for(TEST_SLEEP_TIME);
  return std::make_pair(server, result);
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

  auto surl = p7r::util::url::parse(td.dgram_first);
  surl.query["behaviour"] = "datagram";
  auto curl = p7r::util::url::parse(td.dgram_second);
  curl.query["behaviour"] = "datagram";

  auto res = setup_dgram_connection(td.type, surl, {curl});

  auto server = res.first;
  auto client = res.second[0];

  // Communications
  send_message_dgram(client, server);
  send_message_dgram(server, client);
}



TEST_P(ConnectorDGram, peek_from_send)
{
  // Tests for "datagram" connectors, i.e. connectors that allow synchronous,
  // un-reliable delivery.
  auto td = GetParam();

  auto surl = p7r::util::url::parse(td.dgram_first);
  surl.query["behaviour"] = "datagram";
  auto curl = p7r::util::url::parse(td.dgram_second);
  curl.query["behaviour"] = "datagram";

  auto res = setup_dgram_connection(td.type, surl, {curl});

  auto server = res.first;
  auto client = res.second[0];

  // Communications
  peek_message_dgram(client, server);
  peek_message_dgram(server, client);
}



TEST_P(ConnectorDGram, multiple_clients_blocking)
{
  // Test multiple clients connect to a single server, and can exchange
  // messages.
  auto td = GetParam();

  auto surl = p7r::util::url::parse(td.dgram_first);
  surl.query["behaviour"] = "datagram";
  auto curl1 = p7r::util::url::parse(td.dgram_second);
  curl1.query["behaviour"] = "datagram";
  auto curl2 = p7r::util::url::parse(td.dgram_third);
  curl2.query["behaviour"] = "datagram";

  auto res = setup_dgram_connection(td.type, surl, {curl1, curl2});

  auto server = res.first;
  auto client1 = res.second[0];
  auto client2 = res.second[1];

  // Communications #1 and #2
  send_message_dgram(client1, server, 1);
  send_message_dgram(server, client1, 2);

  send_message_dgram(client2, server, 3);
  send_message_dgram(server, client2, 4);
}




TEST_P(ConnectorDGram, multiple_clients_async)
{
  // Test multiple clients connect to a single server, and can exchange
  // messages.
  auto td = GetParam();

  auto surl = p7r::util::url::parse(td.dgram_first);
  surl.query["behaviour"] = "datagram";
  auto curl1 = p7r::util::url::parse(td.dgram_second);
  curl1.query["behaviour"] = "datagram";
  auto curl2 = p7r::util::url::parse(td.dgram_third);
  curl2.query["behaviour"] = "datagram";

  auto res = setup_dgram_connection(td.type, surl, {curl1, curl2});

  auto server = res.first;
  auto client1 = res.second[0];
  auto client2 = res.second[1];

  // Schedule all the reads/writes
  std::multimap<p7r::peer_address, std::string> result;

  p7r::scheduler sched(test_env->api, 0);
  send_message_dgram_async(0, result, client1, server, sched);
  send_message_dgram_async(1, result, server, client1, sched);
  send_message_dgram_async(2, result, client2, server, sched);
  send_message_dgram_async(3, result, server, client2, sched);

  // Allow the scheduler to do its thing.
  sched.process_events(TEST_SLEEP_TIME);
  sched.process_events(TEST_SLEEP_TIME);
  sched.process_events(TEST_SLEEP_TIME);
  sched.process_events(TEST_SLEEP_TIME);

  // Ensure all results have been written.

  // The server should have received a message from both clients.
  auto range = result.equal_range(server.peer_addr());
  ASSERT_NE(range.first, result.end());
  ASSERT_NE(range.first, range.second);

  std::set<std::string> tmp;
  for (auto iter = range.first ; iter != range.second ; ++iter) {
    tmp.insert(iter->second);
  }
  ASSERT_NE(tmp.end(), tmp.find("Hello, world! [1]"));
  ASSERT_NE(tmp.end(), tmp.find("Hello, world! [3]"));

  // First client should have got '[0]'
  range = result.equal_range(client1.peer_addr());
  ASSERT_NE(range.first, result.end());
  ASSERT_NE(range.first, range.second);

  tmp.clear();
  for (auto iter = range.first ; iter != range.second ; ++iter) {
    tmp.insert(iter->second);
  }
  ASSERT_NE(tmp.end(), tmp.find("Hello, world! [0]"));

  // Second client should have got '[2]'
  range = result.equal_range(client2.peer_addr());
  ASSERT_NE(range.first, result.end());
  ASSERT_NE(range.first, range.second);

  tmp.clear();
  for (auto iter = range.first ; iter != range.second ; ++iter) {
    tmp.insert(iter->second);
  }
  ASSERT_NE(tmp.end(), tmp.find("Hello, world! [2]"));
}



INSTANTIATE_TEST_SUITE_P(net, ConnectorDGram,
    testing::ValuesIn(dgram_tests),
    connector_name<dgram_test_data>);


/*****************************************************************************
 * ConnectorMisc
 */

TEST(ConnectorMisc, anon_connector)
{
  // Anonymous pipes are special in that they need only one connector for
  // communications.
  p7r::connector conn{test_env->api, "anon://"};
  ASSERT_EQ(p7r::CT_ANON, conn.type());

  ASSERT_FALSE(conn.listening());
  ASSERT_FALSE(conn.connected());
  ASSERT_FALSE(conn.communicating());

  ASSERT_EQ(p7r::ERR_SUCCESS, conn.listen());

  ASSERT_TRUE(conn.listening());
  ASSERT_TRUE(conn.connected());
  ASSERT_TRUE(conn.communicating());

  std::string msg = "hello, world!";
  size_t amount = 0;
  ASSERT_EQ(p7r::ERR_SUCCESS, conn.write(msg.c_str(), msg.size(), amount));
  ASSERT_EQ(msg.size(), amount);

  std::vector<char> result;
  result.resize(2 * msg.size(), '\0');
  ASSERT_EQ(p7r::ERR_SUCCESS, conn.read(&result[0], result.capacity(),
        amount));
  ASSERT_EQ(msg.size(), amount);

  for (size_t i = 0 ; i < msg.size() ; ++i) {
    ASSERT_EQ(msg[i], result[i]);
  }
}

