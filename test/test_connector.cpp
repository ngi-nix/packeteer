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

#include <cppunit/extensions/HelperMacros.h>

#include <unistd.h>

#include <cstring>

#include <stdexcept>

#include <twine/chrono.h>

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

} // anonymous namespace

class ConnectorTest
    : public CppUnit::TestFixture
{
public:
  CPPUNIT_TEST_SUITE(ConnectorTest);

    CPPUNIT_TEST(testAddressParsing);
    CPPUNIT_TEST(testValueSemantics);
    CPPUNIT_TEST(testAnonConnector);
    CPPUNIT_TEST(testLocalConnector);
    CPPUNIT_TEST(testPipeConnector);
    CPPUNIT_TEST(testTCPv4Connector);
    CPPUNIT_TEST(testTCPv6Connector);
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

    connector copy = original;
    CPPUNIT_ASSERT_EQUAL(original.type(), copy.type());
    CPPUNIT_ASSERT_EQUAL(original.connect_url(), copy.connect_url());
    CPPUNIT_ASSERT_EQUAL(original.get_read_handle(), copy.get_read_handle());
    CPPUNIT_ASSERT_EQUAL(original.get_write_handle(), copy.get_write_handle());
    CPPUNIT_ASSERT(original == copy);
    CPPUNIT_ASSERT(!(original < copy));

    PACKETEER_VALUES_TEST(copy, original, true);
  }



  void sendMessageStream(connector & sender, connector & receiver)
  {
    std::string msg = "hello, world!";
    size_t amount = 0;
    CPPUNIT_ASSERT_EQUAL(ERR_SUCCESS, sender.write(msg.c_str(), msg.size(), amount));
    CPPUNIT_ASSERT_EQUAL(msg.size(), amount);

    twine::chrono::sleep(twine::chrono::milliseconds(50));

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
    // FIXME use send() and receive()
    // for that, address needs to not return a string (grr)
    std::string msg = "hello, world!";
    size_t amount = 0;
    std::cout << "from " << sender.connect_url() << " to " << receiver.connect_url() << std::endl;
    CPPUNIT_ASSERT_EQUAL(ERR_SUCCESS, sender.send(msg.c_str(), msg.size(), amount,
        receiver.peer_addr()));
    CPPUNIT_ASSERT_EQUAL(msg.size(), amount);

    twine::chrono::sleep(twine::chrono::milliseconds(50));

    std::vector<char> result;
    result.reserve(2 * msg.size());
    packeteer::peer_address sendaddr;
    std::cout << "receiving..." << std::endl;
    CPPUNIT_ASSERT_EQUAL(ERR_SUCCESS, receiver.receive(&result[0], result.capacity(),
          amount, sendaddr));
    CPPUNIT_ASSERT_EQUAL(msg.size(), amount);
    std::cout << "sender socket address: " << sender.peer_addr() << " <> " << sendaddr << std::endl;
    CPPUNIT_ASSERT_EQUAL(sender.peer_addr(), sendaddr);

    for (size_t i = 0 ; i < msg.size() ; ++i) {
      CPPUNIT_ASSERT_EQUAL(msg[i], result[i]);
    }
  }



  void testStreamConnector(connector_type expected_type, std::string const & addr)
  {
    // Tests for "stream" connectors, i.e. connectors that allow synchronous,
    // reliable delivery.

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

    twine::chrono::sleep(twine::chrono::milliseconds(50));

    // Client
    connector client(url);
    CPPUNIT_ASSERT_EQUAL(expected_type, client.type());

    CPPUNIT_ASSERT(!client.listening());
    CPPUNIT_ASSERT(!client.connected());

    CPPUNIT_ASSERT_EQUAL(ERR_SUCCESS, client.connect());
    connector server_conn = server.accept();

    twine::chrono::sleep(twine::chrono::milliseconds(50));

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

    twine::chrono::sleep(twine::chrono::milliseconds(50));

    // Client
    connector client(curl);
    CPPUNIT_ASSERT_EQUAL(expected_type, client.type());

    CPPUNIT_ASSERT(!client.listening());
    CPPUNIT_ASSERT(!client.connected());

    // FIXME not sure?
    CPPUNIT_ASSERT_EQUAL(ERR_SUCCESS, client.listen());
//    CPPUNIT_ASSERT_EQUAL(ERR_SUCCESS, client.connect());

    CPPUNIT_ASSERT(client.listening());
    CPPUNIT_ASSERT(!client.connected());

    twine::chrono::sleep(twine::chrono::milliseconds(50));

    // Communications
    std::cout << "send(client, server)" << std::endl;
    sendMessageDGram(client, server);
    std::cout << "send(server, client)" << std::endl;
    std::cout << "client address " << client.connect_url() << std::endl;
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



  void testLocalConnector()
  {
    // Local sockets are "stream" connectors
    testStreamConnector(CT_LOCAL, "local:///tmp/test-connector-local-stream");
    // FIXME testDGramConnector(CT_LOCAL, "local:///tmp/test-connector-local-dgram");
  }



  void testPipeConnector()
  {
    // Named pipes are "stream" connectors
    testStreamConnector(CT_PIPE, "pipe:///tmp/test-connector-pipe");
  }



  void testTCPv4Connector()
  {
    // TCP over IPv4 to localhost
    testStreamConnector(CT_TCP4, "tcp4://127.0.0.1:54321");
  }



  void testTCPv6Connector()
  {
    // TCP over IPv6 to localhost
    testStreamConnector(CT_TCP6, "tcp6://[::1]:54321");
  }



  void testUDPv4Connector()
  {
    // UDP over IPv4 to localhost
    // TODO
    // * listening sockets can send/receive as expected
    // * connected sockets can send/receive to one peer
    // * unconnected, non-listening sockets can receive, but the sender port will be transient
    // * check how this gets transferred to local://
    testDGramConnector(CT_UDP4, "udp4://127.0.0.1:54321", "udp4://127.0.0.1:54322");
  }



  void testUDPv6Connector()
  {
    // UDP over IPv6 to localhost
    testDGramConnector(CT_UDP6, "udp6://[::1]:54321", "udp6://[::1]:54322");
  }
};


CPPUNIT_TEST_SUITE_REGISTRATION(ConnectorTest);
