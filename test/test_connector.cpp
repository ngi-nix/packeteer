/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2014 Unwesen Ltd.
 * Copyright (c) 2015-2017 Jens Finkhaeuser.
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


using namespace packeteer;

namespace {

struct test_data
{
  char const *              address;
  bool                      valid;
  connector::connector_type type;
} tests[] = {
  // Garbage
  { "foo", false, connector::CT_UNSPEC },
  { "foo:", false, connector::CT_UNSPEC },
  { "foo://", false, connector::CT_UNSPEC },
  { "foo:///some/path", false, connector::CT_UNSPEC },
  { "foo://123.123.133.123:12", false, connector::CT_UNSPEC },
  { "tcp://foo", false, connector::CT_UNSPEC },
  { "tcp4://foo", false, connector::CT_UNSPEC },
  { "tcp6://foo", false, connector::CT_UNSPEC },
  { "udp://foo", false, connector::CT_UNSPEC },
  { "udp4://foo", false, connector::CT_UNSPEC },
  { "udp6://foo", false, connector::CT_UNSPEC },
  { "file://", false, connector::CT_UNSPEC },
  { "ipc://", false, connector::CT_UNSPEC },
  { "pipe://", false, connector::CT_UNSPEC },
  { "anon://anything/here", false, connector::CT_UNSPEC },

  // IPv4 hosts
  { "tcp://192.168.0.1",      true, connector::CT_TCP },
  { "tcp://192.168.0.1:8080", true, connector::CT_TCP },
  { "tCp://192.168.0.1",      true, connector::CT_TCP },
  { "tcP://192.168.0.1:8080", true, connector::CT_TCP },

  { "tcp4://192.168.0.1",      true, connector::CT_TCP4 },
  { "tcp4://192.168.0.1:8080", true, connector::CT_TCP4 },
  { "tCp4://192.168.0.1",      true, connector::CT_TCP4 },
  { "tcP4://192.168.0.1:8080", true, connector::CT_TCP4 },

  { "tcp4://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    false, connector::CT_UNSPEC, },
  { "tcp4://2001:0db8:85a3:0:0:8a2e:0370:7334",          false, connector::CT_UNSPEC, },
  { "tcp4://2001:0db8:85a3::8a2e:0370:7334",             false, connector::CT_UNSPEC, },
  { "Tcp4://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    false, connector::CT_UNSPEC, },
  { "tCp4://2001:0db8:85a3:0:0:8a2e:0370:7334",          false, connector::CT_UNSPEC, },
  { "tcP4://2001:0db8:85a3::8a2e:0370:7334",             false, connector::CT_UNSPEC, },

  { "udp://192.168.0.1",      true, connector::CT_UDP },
  { "udp://192.168.0.1:8080", true, connector::CT_UDP },
  { "uDp://192.168.0.1",      true, connector::CT_UDP },
  { "udP://192.168.0.1:8080", true, connector::CT_UDP },

  { "udp4://192.168.0.1",      true, connector::CT_UDP4 },
  { "udp4://192.168.0.1:8080", true, connector::CT_UDP4 },
  { "uDp4://192.168.0.1",      true, connector::CT_UDP4 },
  { "udP4://192.168.0.1:8080", true, connector::CT_UDP4 },

  { "udp4://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    false, connector::CT_UNSPEC, },
  { "udp4://2001:0db8:85a3:0:0:8a2e:0370:7334",          false, connector::CT_UNSPEC, },
  { "udp4://2001:0db8:85a3::8a2e:0370:7334",             false, connector::CT_UNSPEC, },
  { "Udp4://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    false, connector::CT_UNSPEC, },
  { "uDp4://2001:0db8:85a3:0:0:8a2e:0370:7334",          false, connector::CT_UNSPEC, },
  { "udP4://2001:0db8:85a3::8a2e:0370:7334",             false, connector::CT_UNSPEC, },

  // IPv6 hosts
  { "tcp://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true, connector::CT_TCP, },
  { "tcp://2001:0db8:85a3:0:0:8a2e:0370:7334",          true, connector::CT_TCP, },
  { "tcp://2001:0db8:85a3::8a2e:0370:7334",             true, connector::CT_TCP, },
  { "Tcp://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true, connector::CT_TCP, },
  { "tCp://2001:0db8:85a3:0:0:8a2e:0370:7334",          true, connector::CT_TCP, },
  { "tcP://2001:0db8:85a3::8a2e:0370:7334",             true, connector::CT_TCP, },

  { "tcp6://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true, connector::CT_TCP6, },
  { "tcp6://2001:0db8:85a3:0:0:8a2e:0370:7334",          true, connector::CT_TCP6, },
  { "tcp6://2001:0db8:85a3::8a2e:0370:7334",             true, connector::CT_TCP6, },
  { "Tcp6://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true, connector::CT_TCP6, },
  { "tCp6://2001:0db8:85a3:0:0:8a2e:0370:7334",          true, connector::CT_TCP6, },
  { "tcP6://2001:0db8:85a3::8a2e:0370:7334",             true, connector::CT_TCP6, },

  { "tcp6://192.168.0.1",      false, connector::CT_UNSPEC },
  { "tcp6://192.168.0.1:8080", false, connector::CT_UNSPEC },
  { "tCp6://192.168.0.1",      false, connector::CT_UNSPEC },
  { "tcP6://192.168.0.1:8080", false, connector::CT_UNSPEC },

  { "udp://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true, connector::CT_UDP, },
  { "udp://2001:0db8:85a3:0:0:8a2e:0370:7334",          true, connector::CT_UDP, },
  { "udp://2001:0db8:85a3::8a2e:0370:7334",             true, connector::CT_UDP, },
  { "Udp://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true, connector::CT_UDP, },
  { "uDp://2001:0db8:85a3:0:0:8a2e:0370:7334",          true, connector::CT_UDP, },
  { "udP://2001:0db8:85a3::8a2e:0370:7334",             true, connector::CT_UDP, },

  { "udp6://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true, connector::CT_UDP6, },
  { "udp6://2001:0db8:85a3:0:0:8a2e:0370:7334",          true, connector::CT_UDP6, },
  { "udp6://2001:0db8:85a3::8a2e:0370:7334",             true, connector::CT_UDP6, },
  { "Udp6://2001:0db8:85a3:0000:0000:8a2e:0370:7334",    true, connector::CT_UDP6, },
  { "uDp6://2001:0db8:85a3:0:0:8a2e:0370:7334",          true, connector::CT_UDP6, },
  { "udP6://2001:0db8:85a3::8a2e:0370:7334",             true, connector::CT_UDP6, },

  { "udp6://192.168.0.1",      false, connector::CT_UNSPEC },
  { "udp6://192.168.0.1:8080", false, connector::CT_UNSPEC },
  { "udP6://192.168.0.1",      false, connector::CT_UNSPEC },
  { "uDp6://192.168.0.1:8080", false, connector::CT_UNSPEC },

  // All other types require path names. There's not much common
  // about path names, so our only requirement is that it exists.
  { "local://foo", true, connector::CT_LOCAL },
  { "pipe://foo", true, connector::CT_PIPE },
  { "anon://", true, connector::CT_ANON },

};

} // anonymous namespace

class ConnectorTest
    : public CppUnit::TestFixture
{
public:
  CPPUNIT_TEST_SUITE(ConnectorTest);

    CPPUNIT_TEST(testAddressParsing);
    CPPUNIT_TEST(testCopySemantics);
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



  void testCopySemantics()
  {
    // We'll use an anon connector, because they're simplest.
    connector original("anon://");
    CPPUNIT_ASSERT_EQUAL(connector::CT_ANON, original.type());

    connector copy = original;
    CPPUNIT_ASSERT_EQUAL(original.type(), copy.type());
    CPPUNIT_ASSERT_EQUAL(original.address(), copy.address());
    CPPUNIT_ASSERT_EQUAL(original.get_read_handle(), copy.get_read_handle());
    CPPUNIT_ASSERT_EQUAL(original.get_write_handle(), copy.get_write_handle());
    CPPUNIT_ASSERT(original == copy);
    CPPUNIT_ASSERT(!(original < copy));

    original.swap(copy);
    CPPUNIT_ASSERT(original == copy);

    std::swap(original, copy);
    CPPUNIT_ASSERT(original == copy);

    CPPUNIT_ASSERT(std::equal_to<connector>()(original, copy));
    CPPUNIT_ASSERT(!std::less<connector>()(original, copy));
    CPPUNIT_ASSERT_EQUAL(std::hash<connector>()(original), copy.hash());
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



  void testStreamConnector(connector::connector_type expected_type,
      std::string const & addr)
  {
    // Tests for "stream" connectors, i.e. connectors that allow synchronous,
    // reliable delivery.

    // Server
    connector server(addr);
    CPPUNIT_ASSERT_EQUAL(expected_type, server.type());

    CPPUNIT_ASSERT(!server.listening());
    CPPUNIT_ASSERT(!server.connected());

    CPPUNIT_ASSERT_EQUAL(ERR_SUCCESS, server.listen());

    CPPUNIT_ASSERT(server.listening());
    CPPUNIT_ASSERT(!server.connected());

    twine::chrono::sleep(twine::chrono::milliseconds(50));

    // Client
    connector client(addr);
    CPPUNIT_ASSERT_EQUAL(expected_type, client.type());

    CPPUNIT_ASSERT(!client.listening());
    CPPUNIT_ASSERT(!client.connected());

    CPPUNIT_ASSERT_EQUAL(ERR_SUCCESS, client.connect());
    connector server_conn = server.accept();

    twine::chrono::sleep(twine::chrono::milliseconds(50));

    CPPUNIT_ASSERT(!client.listening());
    CPPUNIT_ASSERT(client.connected());
    CPPUNIT_ASSERT(server_conn.listening());

    // Communications
    sendMessageStream(client, server_conn);
    sendMessageStream(server_conn, client);
  }



  void testDGramConnector(connector::connector_type expected_type,
      std::string const & addr)
  {
    // Tests for "datagram" connectors, i.e. connectors that allow synchronous,
    // un-reliable delivery.

    // FIXME

    // Server
    connector server(addr);
    CPPUNIT_ASSERT_EQUAL(expected_type, server.type());

    CPPUNIT_ASSERT(!server.listening());
    CPPUNIT_ASSERT(!server.connected());

    CPPUNIT_ASSERT_EQUAL(ERR_SUCCESS, server.listen());

    CPPUNIT_ASSERT(server.listening());
    CPPUNIT_ASSERT(!server.connected());

    twine::chrono::sleep(twine::chrono::milliseconds(50));

    // Client
    connector client(addr);
    CPPUNIT_ASSERT_EQUAL(expected_type, client.type());

    CPPUNIT_ASSERT(!client.listening());
    CPPUNIT_ASSERT(!client.connected());

    CPPUNIT_ASSERT_EQUAL(ERR_SUCCESS, client.connect());
    connector server_conn = server.accept();

    twine::chrono::sleep(twine::chrono::milliseconds(50));

    CPPUNIT_ASSERT(!client.listening());
    CPPUNIT_ASSERT(client.connected());
    CPPUNIT_ASSERT(server_conn.listening());

    // Communications
    sendMessageDGram(client, server_conn);
    sendMessageDGram(server_conn, client);
  }



  void testAnonConnector()
  {
    // Anonymous pipes are special in that they need only one connector for
    // communications.
    connector conn("anon://");
    CPPUNIT_ASSERT_EQUAL(connector::CT_ANON, conn.type());

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
    testStreamConnector(connector::CT_LOCAL, "local:///tmp/test-connector-local");
  }



  void testPipeConnector()
  {
    // Named pipes are "stream" connectors
    testStreamConnector(connector::CT_PIPE, "pipe:///tmp/test-connector-pipe");
  }



  void testTCPv4Connector()
  {
    // TCP over IPv4 to localhost
    testStreamConnector(connector::CT_TCP4, "tcp4://127.0.0.1:54321");
  }



  void testTCPv6Connector()
  {
    // TCP over IPv6 to localhost
    testStreamConnector(connector::CT_TCP6, "tcp6://[::1]:54321");
  }



  void testUDPv4Connector()
  {
    // UDP over IPv4 to localhost
    testDGramConnector(connector::CT_UDP4, "udp4://127.0.0.1:54321");
  }



  void testUDPv6Connector()
  {
    // UDP over IPv6 to localhost
    testDGramConnector(connector::CT_UDP6, "udp6://[::1]:54321");
  }
};


CPPUNIT_TEST_SUITE_REGISTRATION(ConnectorTest);
