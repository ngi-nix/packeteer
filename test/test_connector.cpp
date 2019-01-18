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

#include <packeteer/connector.h>
#include <packeteer/scheduler.h>

#include <cppunit/extensions/HelperMacros.h>

#include <unistd.h>

#include <cstring>

#include <stdexcept>

#include <chrono>
#include <thread>

#include "value_tests.h"


using namespace packeteer;

namespace {

struct test_data
{
  char const *    address;
  bool            valid;
  connector_type  type;
} tests[] = {
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


struct client_post_connect_callback
{
  bool m_connected = false;

  ::packeteer::error_t
  func(uint64_t mask, ::packeteer::error_t error, handle const & h, void *)
  {
    if (!m_connected) {
      m_connected = true;
      LOG(" ***** CONNECTED! " << mask << ":" << error << ":" << h.sys_handle());
    }

    return ERR_SUCCESS;
  }
};



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
  func(uint64_t mask, ::packeteer::error_t error, handle const & h, void *)
  {
    if (!m_conn) {
      LOG(" ***** INCOMING " << mask << ":" << error << ":" << h.sys_handle());
      // The accept() function clears the event.
      m_conn = m_server.accept();
      CPPUNIT_ASSERT(m_conn);
    }
    return ERR_SUCCESS;
  }
};

} // anonymous namespace

class ConnectorTest
    : public CppUnit::TestFixture
{
public:
  CPPUNIT_TEST_SUITE(ConnectorTest);

    CPPUNIT_TEST(testAddressParsing);
    CPPUNIT_TEST(testValueSemantics);
    CPPUNIT_TEST(testDefaultConstructed);

    CPPUNIT_TEST(testAnonConnector);

    // Stream connectors blocking and non-blocking
    CPPUNIT_TEST(testLocalConnectorBlocking);
    CPPUNIT_TEST(testLocalConnectorNonBlocking);
    CPPUNIT_TEST(testLocalConnectorDGram);
    CPPUNIT_TEST(testPipeConnectorBlocking);
    CPPUNIT_TEST(testPipeConnectorNonBlocking);
    CPPUNIT_TEST(testTCPv4ConnectorBlocking);
    CPPUNIT_TEST(testTCPv4ConnectorNonBlocking);
    CPPUNIT_TEST(testTCPv6ConnectorBlocking);
    CPPUNIT_TEST(testTCPv6ConnectorNonBlocking);

    CPPUNIT_TEST(testUDPv4Connector);
    CPPUNIT_TEST(testUDPv6Connector);

  CPPUNIT_TEST_SUITE_END();

private:

  void testAddressParsing()
  {
    for (size_t i = 0 ; i < sizeof(tests) / sizeof(test_data) ; ++i) {
      // std::cout << "test: " << tests[i].address << std::endl;

      if (tests[i].valid) {
        CPPUNIT_ASSERT_NO_THROW(auto c = connector(tests[i].address));
        auto c = connector(tests[i].address);
        CPPUNIT_ASSERT_EQUAL(tests[i].type, c.type());
      }
      else {
        CPPUNIT_ASSERT_THROW(auto c = connector(tests[i].address), std::runtime_error);
      }
    }
  }



  void testValueSemantics()
  {
    // We'll use an anon connector, because they're simplest.
    connector original("anon://");
    CPPUNIT_ASSERT_EQUAL(CT_ANON, original.type());
    CPPUNIT_ASSERT(original);

    connector copy = original;
    CPPUNIT_ASSERT_EQUAL(original.type(), copy.type());
    CPPUNIT_ASSERT_EQUAL(original.connect_url(), copy.connect_url());
    CPPUNIT_ASSERT_EQUAL(original.get_read_handle(), copy.get_read_handle());
    CPPUNIT_ASSERT_EQUAL(original.get_write_handle(), copy.get_write_handle());
    CPPUNIT_ASSERT(original == copy);
    CPPUNIT_ASSERT(!(original < copy));

    PACKETEER_VALUES_TEST(copy, original, true);
  }


  void testDefaultConstructed()
  {
    // Default constructed connectors should vaguely work.
    connector conn;
    CPPUNIT_ASSERT_EQUAL(CT_UNSPEC, conn.type());
    CPPUNIT_ASSERT(!conn);

    CPPUNIT_ASSERT_THROW(auto url = conn.connect_url(), exception);

    // Most functions should just return ERR_INITIALIZATION
    bool mode = false;
    CPPUNIT_ASSERT_EQUAL(ERR_INITIALIZATION, conn.get_blocking_mode(mode));

    // Comparison should always yield the unspecified connector to be smaller.
    connector conn2;
    CPPUNIT_ASSERT(!conn2);
    CPPUNIT_ASSERT(conn == conn2);
    CPPUNIT_ASSERT(conn2 == conn);

    connector anon("anon://");
    CPPUNIT_ASSERT(anon);
    CPPUNIT_ASSERT(conn < anon);
    CPPUNIT_ASSERT(anon > conn);

    // Assigning does work, though
    conn = anon;
    CPPUNIT_ASSERT(conn);
    CPPUNIT_ASSERT(conn == anon);
    CPPUNIT_ASSERT(anon == conn);

    CPPUNIT_ASSERT(!(conn == conn2));
    CPPUNIT_ASSERT(conn2 < conn);
    CPPUNIT_ASSERT(conn > conn2);
  }



  void sendMessageStream(connector & sender, connector & receiver)
  {
    std::string msg = "hello, world!";
    size_t amount = 0;
    CPPUNIT_ASSERT_EQUAL(ERR_SUCCESS, sender.write(msg.c_str(), msg.size(), amount));
    CPPUNIT_ASSERT_EQUAL(msg.size(), amount);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::vector<char> result;
    result.reserve(2 * msg.size());
    CPPUNIT_ASSERT_EQUAL(ERR_SUCCESS, receiver.read(&result[0], result.capacity(),
          amount));
    CPPUNIT_ASSERT_EQUAL(msg.size(), amount);

    for (size_t i = 0 ; i < msg.size() ; ++i) {
      CPPUNIT_ASSERT_EQUAL(msg[i], result[i]);
    }
  }



  void sendMessageDGram(connector & sender, connector & receiver)
  {
    std::string msg = "hello, world!";
    size_t amount = 0;
    CPPUNIT_ASSERT_EQUAL(ERR_SUCCESS, sender.send(msg.c_str(), msg.size(), amount,
        receiver.peer_addr()));
    CPPUNIT_ASSERT_EQUAL(msg.size(), amount);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::vector<char> result;
    result.reserve(2 * msg.size());
    packeteer::peer_address sendaddr;
    CPPUNIT_ASSERT_EQUAL(ERR_SUCCESS, receiver.receive(&result[0], result.capacity(),
          amount, sendaddr));
    CPPUNIT_ASSERT_EQUAL(msg.size(), amount);
    CPPUNIT_ASSERT_EQUAL(sender.peer_addr(), sendaddr);

    for (size_t i = 0 ; i < msg.size() ; ++i) {
      CPPUNIT_ASSERT_EQUAL(msg[i], result[i]);
    }
  }



  void testBlockingStreamConnector(connector_type expected_type, std::string const & addr)
  {
    // Tests for "stream" connectors, i.e. connectors that allow synchronous,
    // reliable delivery - in blocking mode, making the setup/teardown very simple.

    auto url = ::packeteer::util::url::parse(addr);
    url.query["behaviour"] = "stream";

    // Server
    connector server(url);
    CPPUNIT_ASSERT_EQUAL(expected_type, server.type());

    CPPUNIT_ASSERT(!server.listening());
    CPPUNIT_ASSERT(!server.connected());

    CPPUNIT_ASSERT_EQUAL(ERR_SUCCESS, server.listen());

    CPPUNIT_ASSERT(server.listening());
    CPPUNIT_ASSERT(!server.connected());

    bool mode = false;
    CPPUNIT_ASSERT_EQUAL(ERR_SUCCESS, server.get_blocking_mode(mode));
    CPPUNIT_ASSERT_EQUAL(true, mode);
    CPPUNIT_ASSERT_EQUAL(CB_STREAM, server.get_behaviour());

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Client
    connector client(url);
    CPPUNIT_ASSERT_EQUAL(expected_type, client.type());

    CPPUNIT_ASSERT(!client.listening());
    CPPUNIT_ASSERT(!client.connected());

    CPPUNIT_ASSERT_EQUAL(ERR_SUCCESS, client.connect());
    connector server_conn = server.accept();

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    CPPUNIT_ASSERT(!client.listening());
    CPPUNIT_ASSERT(client.connected());
    CPPUNIT_ASSERT(server_conn.listening());

    CPPUNIT_ASSERT_EQUAL(ERR_SUCCESS, server_conn.get_blocking_mode(mode));
    CPPUNIT_ASSERT_EQUAL(true, mode);
    CPPUNIT_ASSERT_EQUAL(CB_STREAM, server_conn.get_behaviour());

    CPPUNIT_ASSERT_EQUAL(ERR_SUCCESS, client.get_blocking_mode(mode));
    CPPUNIT_ASSERT_EQUAL(true, mode);
    CPPUNIT_ASSERT_EQUAL(CB_STREAM, client.get_behaviour());

    // Communications
    sendMessageStream(client, server_conn);
    sendMessageStream(server_conn, client);
  }



  void testNonBlockingStreamConnector(connector_type expected_type, std::string const & addr)
  {
    // Tests for "stream" connectors, i.e. connectors that allow synchronous,
    // reliable delivery - in non-blocking mode, meaning we need to react to
    // events with the scheduler.

    auto url = ::packeteer::util::url::parse(addr);
    url.query["behaviour"] = "stream";

    // Server
    connector server(url);
    CPPUNIT_ASSERT_EQUAL(expected_type, server.type());

    CPPUNIT_ASSERT(!server.listening());
    CPPUNIT_ASSERT(!server.connected());

    CPPUNIT_ASSERT_EQUAL(ERR_SUCCESS, server.listen());

    CPPUNIT_ASSERT(server.listening());
    CPPUNIT_ASSERT(!server.connected());

    bool mode = false;
    CPPUNIT_ASSERT_EQUAL(ERR_SUCCESS, server.get_blocking_mode(mode));
    CPPUNIT_ASSERT_EQUAL(false, mode);
    CPPUNIT_ASSERT_EQUAL(CB_STREAM, server.get_behaviour());

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Client
    connector client(url);
    CPPUNIT_ASSERT_EQUAL(expected_type, client.type());

    CPPUNIT_ASSERT(!client.listening());
    CPPUNIT_ASSERT(!client.connected());

    // Connecting must result in ERR_ASYNC. We use a scheduler run to
    // understand when the connection attempt was finished.
    scheduler sched(1);
    server_connect_callback server_struct(server);
    auto server_cb = make_callback(&server_struct, &server_connect_callback::func);
    sched.register_handle(PEV_IO_READ|PEV_IO_WRITE, server.get_read_handle(),
        server_cb);

    // Give scheduler a chance to register handlers
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    CPPUNIT_ASSERT_EQUAL(ERR_ASYNC, client.connect());

    client_post_connect_callback client_struct;
    auto client_cb = make_callback(&client_struct, &client_post_connect_callback::func);
    sched.register_handle(PEV_IO_READ|PEV_IO_WRITE, client.get_read_handle(),
        client_cb);

    // Wait for all callbacks to be invoked.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // After the sleep, the server conn and client conn should both
    // be ready to roll.
    CPPUNIT_ASSERT(server_struct.m_conn);
    CPPUNIT_ASSERT(client_struct.m_connected);

    connector server_conn = server_struct.m_conn;

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    CPPUNIT_ASSERT(!client.listening());
    CPPUNIT_ASSERT(client.connected());
    CPPUNIT_ASSERT(server_conn.listening());

    CPPUNIT_ASSERT_EQUAL(ERR_SUCCESS, server_conn.get_blocking_mode(mode));
    CPPUNIT_ASSERT_EQUAL(false, mode);
    CPPUNIT_ASSERT_EQUAL(CB_STREAM, server_conn.get_behaviour());

    CPPUNIT_ASSERT_EQUAL(ERR_SUCCESS, client.get_blocking_mode(mode));
    CPPUNIT_ASSERT_EQUAL(false, mode);
    CPPUNIT_ASSERT_EQUAL(CB_STREAM, client.get_behaviour());

    // Communications
    sendMessageStream(client, server_conn);
    sendMessageStream(server_conn, client);
  }



  void testDGramConnector(connector_type expected_type, std::string const & saddr, std::string const & caddr)
  {
    // Tests for "datagram" connectors, i.e. connectors that allow synchronous,
    // un-reliable delivery.

    auto surl = ::packeteer::util::url::parse(saddr);
    surl.query["behaviour"] = "datagram";
    auto curl = ::packeteer::util::url::parse(caddr);
    curl.query["behaviour"] = "datagram";

    // Server
    connector server(surl);
    CPPUNIT_ASSERT_EQUAL(expected_type, server.type());

    CPPUNIT_ASSERT(!server.listening());
    CPPUNIT_ASSERT(!server.connected());

    CPPUNIT_ASSERT_EQUAL(ERR_SUCCESS, server.listen());

    CPPUNIT_ASSERT(server.listening());
    CPPUNIT_ASSERT(!server.connected());

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Client
    connector client(curl);
    CPPUNIT_ASSERT_EQUAL(expected_type, client.type());

    CPPUNIT_ASSERT(!client.listening());
    CPPUNIT_ASSERT(!client.connected());

    CPPUNIT_ASSERT_EQUAL(ERR_SUCCESS, client.listen());

    CPPUNIT_ASSERT(client.listening());
    CPPUNIT_ASSERT(!client.connected());

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Communications
    sendMessageDGram(client, server);
    sendMessageDGram(server, client);
  }



  void testAnonConnector()
  {
    // Anonymous pipes are special in that they need only one connector for
    // communications.
    connector conn("anon://");
    CPPUNIT_ASSERT_EQUAL(CT_ANON, conn.type());

    CPPUNIT_ASSERT(!conn.listening());
    CPPUNIT_ASSERT(!conn.connected());

    CPPUNIT_ASSERT_EQUAL(ERR_SUCCESS, conn.listen());

    CPPUNIT_ASSERT(conn.listening());
    CPPUNIT_ASSERT(conn.connected());

    std::string msg = "hello, world!";
    size_t amount = 0;
    CPPUNIT_ASSERT_EQUAL(ERR_SUCCESS, conn.write(msg.c_str(), msg.size(), amount));
    CPPUNIT_ASSERT_EQUAL(msg.size(), amount);

    std::vector<char> result;
    result.reserve(2 * msg.size());
    CPPUNIT_ASSERT_EQUAL(ERR_SUCCESS, conn.read(&result[0], result.capacity(),
          amount));
    CPPUNIT_ASSERT_EQUAL(msg.size(), amount);

    for (size_t i = 0 ; i < msg.size() ; ++i) {
      CPPUNIT_ASSERT_EQUAL(msg[i], result[i]);
    }
  }



  void testLocalConnectorBlocking()
  {
    testBlockingStreamConnector(CT_LOCAL, "local:///tmp/test-connector-local-stream-block?blocking=1");
  }

  void testLocalConnectorNonBlocking()
  {
    testNonBlockingStreamConnector(CT_LOCAL, "local:///tmp/test-connector-local-stream-noblock");
  }

  void testLocalConnectorDGram()
  {
    testDGramConnector(CT_LOCAL, "local:///tmp/test-connector-local-dgram-first",
        "local:///tmp/test-connector-local-dgram-second");
  }


  void testPipeConnectorBlocking()
  {
    testBlockingStreamConnector(CT_PIPE, "pipe:///tmp/test-connector-pipe-block?blocking=1");
  }

  void testPipeConnectorNonBlocking()
  {
    testNonBlockingStreamConnector(CT_PIPE, "pipe:///tmp/test-connector-pipe-noblock");
  }



  void testTCPv4ConnectorBlocking()
  {
    testBlockingStreamConnector(CT_TCP4, "tcp4://127.0.0.1:54321?blocking=1");
  }

  void testTCPv4ConnectorNonBlocking()
  {
    testNonBlockingStreamConnector(CT_TCP4, "tcp4://127.0.0.1:54321");
  }



  void testTCPv6ConnectorBlocking()
  {
    testBlockingStreamConnector(CT_TCP6, "tcp6://[::1]:54321?blocking=1");
  }

  void testTCPv6ConnectorNonBlocking()
  {
    testNonBlockingStreamConnector(CT_TCP6, "tcp6://[::1]:54321");
  }



  void testUDPv4Connector()
  {
    // UDP over IPv4 to localhost
    testDGramConnector(CT_UDP4, "udp4://127.0.0.1:54321", "udp4://127.0.0.1:54322");
  }



  void testUDPv6Connector()
  {
    // UDP over IPv6 to localhost
    testDGramConnector(CT_UDP6, "udp6://[::1]:54321", "udp6://[::1]:54322");
  }
};


CPPUNIT_TEST_SUITE_REGISTRATION(ConnectorTest);
