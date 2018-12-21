/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2018-2019 Jens Finkhaeuser.
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

#include <packeteer/util/url.h>

#include <cppunit/extensions/HelperMacros.h>

namespace util = packeteer::util;

namespace {

} // anonymous namespace

class URLTest
    : public CppUnit::TestFixture
{
public:
  CPPUNIT_TEST_SUITE(URLTest);

    CPPUNIT_TEST(testComplete);
    CPPUNIT_TEST(testAuthorityPathAndFragment);
    CPPUNIT_TEST(testAuthorityPathAndQuery);
    CPPUNIT_TEST(testAuthorityQueryAndFragment);
    CPPUNIT_TEST(testPathAndFragment);
    CPPUNIT_TEST(testPathAndQuery);
    CPPUNIT_TEST(testQueryAndFragment);

    CPPUNIT_TEST(testIPAddress);
    CPPUNIT_TEST(testAnon);
    CPPUNIT_TEST(testLocal);
    CPPUNIT_TEST(testPipe);

    CPPUNIT_TEST(testNonBlocking);
    CPPUNIT_TEST(testDatagram);

  CPPUNIT_TEST_SUITE_END();

private:

  void testComplete()
  {
    // With everything
    auto url = util::url::parse("https://finkhaeuser.de/path/to?some=value&simple&other=tRue#myfrag");
    CPPUNIT_ASSERT_EQUAL(std::string("https"), url.scheme);
    CPPUNIT_ASSERT_EQUAL(std::string("finkhaeuser.de"), url.authority);
    CPPUNIT_ASSERT_EQUAL(std::string("/path/to"), url.path);
    CPPUNIT_ASSERT(3 == url.query.size());

    auto iter = url.query.find("some");
    CPPUNIT_ASSERT(url.query.end() != iter);
    CPPUNIT_ASSERT_EQUAL(std::string("value"), iter->second);

    iter = url.query.find("simple");
    CPPUNIT_ASSERT(url.query.end() != iter);
    CPPUNIT_ASSERT_EQUAL(std::string("1"), iter->second);

    iter = url.query.find("other");
    CPPUNIT_ASSERT(url.query.end() != iter);
    CPPUNIT_ASSERT_EQUAL(std::string("1"), iter->second);

    CPPUNIT_ASSERT_EQUAL(std::string("myfrag"), url.fragment);
  }

  void testAuthorityPathAndFragment()
  {
    // With fragment, no query
    auto url = util::url::parse("https://finkhaeuser.de/path/to/#myfrag");
    CPPUNIT_ASSERT_EQUAL(std::string("https"), url.scheme);
    CPPUNIT_ASSERT_EQUAL(std::string("finkhaeuser.de"), url.authority);
    CPPUNIT_ASSERT_EQUAL(std::string("/path/to/"), url.path);
    CPPUNIT_ASSERT(0 == url.query.size());
    CPPUNIT_ASSERT_EQUAL(std::string("myfrag"), url.fragment);

    // With query, no fragment
    url = util::url::parse("https://finkhaeuser.de/path/to?some=value&simple&other=tRue");
    CPPUNIT_ASSERT_EQUAL(std::string("https"), url.scheme);
    CPPUNIT_ASSERT_EQUAL(std::string("finkhaeuser.de"), url.authority);
    CPPUNIT_ASSERT_EQUAL(std::string("/path/to"), url.path);
    CPPUNIT_ASSERT(3 == url.query.size());

    auto iter = url.query.find("some");
    CPPUNIT_ASSERT(url.query.end() != iter);
    CPPUNIT_ASSERT_EQUAL(std::string("value"), iter->second);

    iter = url.query.find("simple");
    CPPUNIT_ASSERT(url.query.end() != iter);
    CPPUNIT_ASSERT_EQUAL(std::string("1"), iter->second);

    iter = url.query.find("other");
    CPPUNIT_ASSERT(url.query.end() != iter);
    CPPUNIT_ASSERT_EQUAL(std::string("1"), iter->second);

    CPPUNIT_ASSERT(0 == url.fragment.length());
  }

  void testAuthorityPathAndQuery()
  {
    // With query, no fragment
    auto url = util::url::parse("https://finkhaeuser.de/path/to?some=value&simple&other=tRue");
    CPPUNIT_ASSERT_EQUAL(std::string("https"), url.scheme);
    CPPUNIT_ASSERT_EQUAL(std::string("finkhaeuser.de"), url.authority);
    CPPUNIT_ASSERT_EQUAL(std::string("/path/to"), url.path);
    CPPUNIT_ASSERT(3 == url.query.size());

    auto iter = url.query.find("some");
    CPPUNIT_ASSERT(url.query.end() != iter);
    CPPUNIT_ASSERT_EQUAL(std::string("value"), iter->second);

    iter = url.query.find("simple");
    CPPUNIT_ASSERT(url.query.end() != iter);
    CPPUNIT_ASSERT_EQUAL(std::string("1"), iter->second);

    iter = url.query.find("other");
    CPPUNIT_ASSERT(url.query.end() != iter);
    CPPUNIT_ASSERT_EQUAL(std::string("1"), iter->second);

    CPPUNIT_ASSERT(0 == url.fragment.length());
  }


  void testAuthorityQueryAndFragment()
  {
    // Query and fragment, but no path
    auto url = util::url::parse("https://finkhaeuser.de?some=value&simple&other=tRue#myfrag");
    CPPUNIT_ASSERT_EQUAL(std::string("https"), url.scheme);
    CPPUNIT_ASSERT_EQUAL(std::string("finkhaeuser.de"), url.authority);
    CPPUNIT_ASSERT(0 == url.path.length());
    CPPUNIT_ASSERT(3 == url.query.size());

    auto iter = url.query.find("some");
    CPPUNIT_ASSERT(url.query.end() != iter);
    CPPUNIT_ASSERT_EQUAL(std::string("value"), iter->second);

    iter = url.query.find("simple");
    CPPUNIT_ASSERT(url.query.end() != iter);
    CPPUNIT_ASSERT_EQUAL(std::string("1"), iter->second);

    iter = url.query.find("other");
    CPPUNIT_ASSERT(url.query.end() != iter);
    CPPUNIT_ASSERT_EQUAL(std::string("1"), iter->second);

    CPPUNIT_ASSERT_EQUAL(std::string("myfrag"), url.fragment);

  }

  void testPathAndFragment()
  {
    // No authority, but path and framgnent
    auto url = util::url::parse("file:///path/to?some=value&simple&other=tRue#myfrag");
    CPPUNIT_ASSERT_EQUAL(std::string("file"), url.scheme);
    CPPUNIT_ASSERT(0 == url.authority.length());
    CPPUNIT_ASSERT_EQUAL(std::string("/path/to"), url.path);
    CPPUNIT_ASSERT(3 == url.query.size());

    auto iter = url.query.find("some");
    CPPUNIT_ASSERT(url.query.end() != iter);
    CPPUNIT_ASSERT_EQUAL(std::string("value"), iter->second);

    iter = url.query.find("simple");
    CPPUNIT_ASSERT(url.query.end() != iter);
    CPPUNIT_ASSERT_EQUAL(std::string("1"), iter->second);

    iter = url.query.find("other");
    CPPUNIT_ASSERT(url.query.end() != iter);
    CPPUNIT_ASSERT_EQUAL(std::string("1"), iter->second);

    CPPUNIT_ASSERT_EQUAL(std::string("myfrag"), url.fragment);

  }

  void testPathAndQuery()
  {
    // No authority, but path and query. No fragment.
    auto url = util::url::parse("file:///path/to#myfrag");
    CPPUNIT_ASSERT_EQUAL(std::string("file"), url.scheme);
    CPPUNIT_ASSERT(0 == url.authority.length());
    CPPUNIT_ASSERT_EQUAL(std::string("/path/to"), url.path);
    CPPUNIT_ASSERT(0 == url.query.size());
    CPPUNIT_ASSERT_EQUAL(std::string("myfrag"), url.fragment);
  }

  void testQueryAndFragment()
  {
    // No authority, no path, just query and fragment.
    auto url = util::url::parse("file://?some=value&simple&other=tRue#myfrag");
    CPPUNIT_ASSERT_EQUAL(std::string("file"), url.scheme);
    CPPUNIT_ASSERT(0 == url.authority.length());
    CPPUNIT_ASSERT(0 == url.path.length());
    CPPUNIT_ASSERT(3 == url.query.size());

    auto iter = url.query.find("some");
    CPPUNIT_ASSERT(url.query.end() != iter);
    CPPUNIT_ASSERT_EQUAL(std::string("value"), iter->second);

    iter = url.query.find("simple");
    CPPUNIT_ASSERT(url.query.end() != iter);
    CPPUNIT_ASSERT_EQUAL(std::string("1"), iter->second);

    iter = url.query.find("other");
    CPPUNIT_ASSERT(url.query.end() != iter);
    CPPUNIT_ASSERT_EQUAL(std::string("1"), iter->second);

    CPPUNIT_ASSERT_EQUAL(std::string("myfrag"), url.fragment);

  }



  void testIPAddress()
  {
    auto url = util::url::parse("TcP4://127.0.0.1:123");
    CPPUNIT_ASSERT_EQUAL(std::string("tcp4"), url.scheme);
    CPPUNIT_ASSERT_EQUAL(std::string("127.0.0.1:123"), url.authority);
    CPPUNIT_ASSERT(0 == url.path.length());
    CPPUNIT_ASSERT(0 == url.query.size());
    CPPUNIT_ASSERT(0 == url.fragment.length());
  }


  void testAnon()
  {
    auto url = util::url::parse("anon://");
    CPPUNIT_ASSERT_EQUAL(std::string("anon"), url.scheme);
    CPPUNIT_ASSERT(0 == url.authority.length());
    CPPUNIT_ASSERT(0 == url.path.length());
    CPPUNIT_ASSERT(0 == url.query.size());
    CPPUNIT_ASSERT(0 == url.fragment.length());

  }

  void testLocal()
  {
    auto url = util::url::parse("local:///foo/bar");
    CPPUNIT_ASSERT_EQUAL(std::string("local"), url.scheme);
    CPPUNIT_ASSERT(0 == url.authority.length());
    CPPUNIT_ASSERT_EQUAL(std::string("/foo/bar"), url.path);
    CPPUNIT_ASSERT(0 == url.query.size());
    CPPUNIT_ASSERT(0 == url.fragment.length());
  }

  void testPipe()
  {
    auto url = util::url::parse("pipe:///foo/bar");
    CPPUNIT_ASSERT_EQUAL(std::string("pipe"), url.scheme);
    CPPUNIT_ASSERT(0 == url.authority.length());
    CPPUNIT_ASSERT_EQUAL(std::string("/foo/bar"), url.path);
    CPPUNIT_ASSERT(0 == url.query.size());
    CPPUNIT_ASSERT(0 == url.fragment.length());

  }


  void testNonBlocking()
  {
    auto url = util::url::parse("pipe:///foo/bar?blocking=false");
    CPPUNIT_ASSERT(1 == url.query.size());

    auto iter = url.query.find("blocking");
    CPPUNIT_ASSERT(url.query.end() != iter);
    CPPUNIT_ASSERT_EQUAL(std::string("0"), iter->second);
  }


  void testDatagram()
  {
    auto url = util::url::parse("pipe:///foo/bar?behaviour=datagram");
    CPPUNIT_ASSERT(1 == url.query.size());

    auto iter = url.query.find("behaviour");
    CPPUNIT_ASSERT(url.query.end() != iter);
    CPPUNIT_ASSERT_EQUAL(std::string("datagram"), iter->second);
  }


};


CPPUNIT_TEST_SUITE_REGISTRATION(URLTest);
