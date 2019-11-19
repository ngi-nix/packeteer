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

#include <gtest/gtest.h>

namespace util = packeteer::util;

TEST(URL, complete)
{
  // With everything
  auto url = util::url::parse("https://finkhaeuser.de/path/to?some=value&simple&other=tRue#myfrag");
  ASSERT_EQ(std::string("https"), url.scheme);
  ASSERT_EQ(std::string("finkhaeuser.de"), url.authority);
  ASSERT_EQ(std::string("/path/to"), url.path);
  ASSERT_EQ(3, url.query.size());

  auto iter = url.query.find("some");
  ASSERT_NE(url.query.end(), iter);
  ASSERT_EQ(std::string("value"), iter->second);

  iter = url.query.find("simple");
  ASSERT_NE(url.query.end(), iter);
  ASSERT_EQ(std::string("1"), iter->second);

  iter = url.query.find("other");
  ASSERT_NE(url.query.end(), iter);
  ASSERT_EQ(std::string("1"), iter->second);

  ASSERT_EQ(std::string("myfrag"), url.fragment);
}


TEST(URL, authority_path_and_fragment)
{
  // With fragment, no query
  auto url = util::url::parse("https://finkhaeuser.de/path/to/#myfrag");
  ASSERT_EQ(std::string("https"), url.scheme);
  ASSERT_EQ(std::string("finkhaeuser.de"), url.authority);
  ASSERT_EQ(std::string("/path/to/"), url.path);
  ASSERT_EQ(0, url.query.size());
  ASSERT_EQ(std::string("myfrag"), url.fragment);

  // With query, no fragment
  url = util::url::parse("https://finkhaeuser.de/path/to?some=value&simple&other=tRue");
  ASSERT_EQ(std::string("https"), url.scheme);
  ASSERT_EQ(std::string("finkhaeuser.de"), url.authority);
  ASSERT_EQ(std::string("/path/to"), url.path);
  ASSERT_EQ(3, url.query.size());

  auto iter = url.query.find("some");
  ASSERT_NE(url.query.end(), iter);
  ASSERT_EQ(std::string("value"), iter->second);

  iter = url.query.find("simple");
  ASSERT_NE(url.query.end(), iter);
  ASSERT_EQ(std::string("1"), iter->second);

  iter = url.query.find("other");
  ASSERT_NE(url.query.end(), iter);
  ASSERT_EQ(std::string("1"), iter->second);

  ASSERT_EQ(0, url.fragment.length());
}


TEST(URL, authority_path_and_query)
{
  // With query, no fragment
  auto url = util::url::parse("https://finkhaeuser.de/path/to?some=value&simple&other=tRue");
  ASSERT_EQ(std::string("https"), url.scheme);
  ASSERT_EQ(std::string("finkhaeuser.de"), url.authority);
  ASSERT_EQ(std::string("/path/to"), url.path);
  ASSERT_EQ(3, url.query.size());

  auto iter = url.query.find("some");
  ASSERT_NE(url.query.end(), iter);
  ASSERT_EQ(std::string("value"), iter->second);

  iter = url.query.find("simple");
  ASSERT_NE(url.query.end(), iter);
  ASSERT_EQ(std::string("1"), iter->second);

  iter = url.query.find("other");
  ASSERT_NE(url.query.end(), iter);
  ASSERT_EQ(std::string("1"), iter->second);

  ASSERT_EQ(0, url.fragment.length());
}


TEST(URL, authority_query_and_fragment)
{
  // Query and fragment, but no path
  auto url = util::url::parse("https://finkhaeuser.de?some=value&simple&other=tRue#myfrag");
  ASSERT_EQ(std::string("https"), url.scheme);
  ASSERT_EQ(std::string("finkhaeuser.de"), url.authority);
  ASSERT_EQ(0, url.path.length());
  ASSERT_EQ(3, url.query.size());

  auto iter = url.query.find("some");
  ASSERT_NE(url.query.end(), iter);
  ASSERT_EQ(std::string("value"), iter->second);

  iter = url.query.find("simple");
  ASSERT_NE(url.query.end(), iter);
  ASSERT_EQ(std::string("1"), iter->second);

  iter = url.query.find("other");
  ASSERT_NE(url.query.end(), iter);
  ASSERT_EQ(std::string("1"), iter->second);

  ASSERT_EQ(std::string("myfrag"), url.fragment);
}

TEST(URL, path_and_fragment)
{
  // No authority, but path and framgnent
  auto url = util::url::parse("file:///path/to?some=value&simple&other=tRue#myfrag");
  ASSERT_EQ(std::string("file"), url.scheme);
  ASSERT_EQ(0, url.authority.length());
  ASSERT_EQ(std::string("/path/to"), url.path);
  ASSERT_EQ(3, url.query.size());

  auto iter = url.query.find("some");
  ASSERT_NE(url.query.end(), iter);
  ASSERT_EQ(std::string("value"), iter->second);

  iter = url.query.find("simple");
  ASSERT_NE(url.query.end(), iter);
  ASSERT_EQ(std::string("1"), iter->second);

  iter = url.query.find("other");
  ASSERT_NE(url.query.end(), iter);
  ASSERT_EQ(std::string("1"), iter->second);

  ASSERT_EQ(std::string("myfrag"), url.fragment);

}

TEST(URL, path_and_query)
{
  // No authority, but path and query. No fragment.
  auto url = util::url::parse("file:///path/to#myfrag");
  ASSERT_EQ(std::string("file"), url.scheme);
  ASSERT_EQ(0, url.authority.length());
  ASSERT_EQ(std::string("/path/to"), url.path);
  ASSERT_EQ(0, url.query.size());
  ASSERT_EQ(std::string("myfrag"), url.fragment);
}

TEST(URL, query_and_fragment)
{
  // No authority, no path, just query and fragment.
  auto url = util::url::parse("file://?some=value&simple&other=tRue#myfrag");
  ASSERT_EQ(std::string("file"), url.scheme);
  ASSERT_EQ(0, url.authority.length());
  ASSERT_EQ(0, url.path.length());
  ASSERT_EQ(3, url.query.size());

  auto iter = url.query.find("some");
  ASSERT_NE(url.query.end(), iter);
  ASSERT_EQ(std::string("value"), iter->second);

  iter = url.query.find("simple");
  ASSERT_NE(url.query.end(), iter);
  ASSERT_EQ(std::string("1"), iter->second);

  iter = url.query.find("other");
  ASSERT_NE(url.query.end(), iter);
  ASSERT_EQ(std::string("1"), iter->second);

  ASSERT_EQ(std::string("myfrag"), url.fragment);
}


TEST(URL, IP_address)
{
  auto url = util::url::parse("TcP4://127.0.0.1:123");
  ASSERT_EQ(std::string("tcp4"), url.scheme);
  ASSERT_EQ(std::string("127.0.0.1:123"), url.authority);
  ASSERT_EQ(0, url.path.length());
  ASSERT_EQ(0, url.query.size());
  ASSERT_EQ(0, url.fragment.length());
}


TEST(URL, anon)
{
  auto url = util::url::parse("anon://");
  ASSERT_EQ(std::string("anon"), url.scheme);
  ASSERT_EQ(0, url.authority.length());
  ASSERT_EQ(0, url.path.length());
  ASSERT_EQ(0, url.query.size());
  ASSERT_EQ(0, url.fragment.length());
}


TEST(URL, local)
{
  auto url = util::url::parse("local:///foo/bar");
  ASSERT_EQ(std::string("local"), url.scheme);
  ASSERT_EQ(0, url.authority.length());
  ASSERT_EQ(std::string("/foo/bar"), url.path);
  ASSERT_EQ(0, url.query.size());
  ASSERT_EQ(0, url.fragment.length());
}


TEST(URL, pipe)
{
  auto url = util::url::parse("pipe:///foo/bar");
  ASSERT_EQ(std::string("pipe"), url.scheme);
  ASSERT_EQ(0, url.authority.length());
  ASSERT_EQ(std::string("/foo/bar"), url.path);
  ASSERT_EQ(0, url.query.size());
  ASSERT_EQ(0, url.fragment.length());
}


TEST(URL, non_blocking)
{
  auto url = util::url::parse("pipe:///foo/bar?blocking=false");
  ASSERT_EQ(1, url.query.size());

  auto iter = url.query.find("blocking");
  ASSERT_NE(url.query.end(), iter);
  ASSERT_EQ(std::string("0"), iter->second);
}


TEST(URL, datagram)
{
  auto url = util::url::parse("pipe:///foo/bar?behaviour=datagram");
  ASSERT_EQ(1, url.query.size());

  auto iter = url.query.find("behaviour");
  ASSERT_NE(url.query.end(), iter);
  ASSERT_EQ(std::string("datagram"), iter->second);
}
