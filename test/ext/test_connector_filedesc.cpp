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

#include <packeteer/ext/connector/filedesc.h>
#include <packeteer/connector.h>

namespace p7r = packeteer;

TEST(ExtConnectorFileDesc, raise_without_registration)
{
  ASSERT_THROW(auto conn = p7r::connector(test_env->api, "filedesc:///stdin"),
    p7r::exception);
}


TEST(ExtConnectorFileDesc, succeed_with_registration)
{
  auto api = packeteer::api::create();
  p7r::ext::register_connector_filedesc(api);
  ASSERT_NO_THROW(auto conn = p7r::connector(api, "filedesc:///stdin"));
  ASSERT_NO_THROW(auto conn = p7r::connector(api, "fd:///stdin"));
}


TEST(ExtConnectorFileDesc, duplicate_anon)
{
  auto api = packeteer::api::create();
  p7r::ext::register_connector_filedesc(api);

  auto anon = p7r::connector{api, "anon://?blocking=true"};
  anon.connect();
  ASSERT_TRUE(anon.connected());
  ASSERT_TRUE(anon.is_blocking());

  // Duplicate read FD
  std::string url = "fd:///" + std::to_string(anon.get_read_handle().sys_handle()) + "?blocking=true";

  auto fd = p7r::connector{api, url};
  ASSERT_TRUE(fd.connected());
  ASSERT_TRUE(fd.is_blocking());

  // Write to anon must make fd readable
  std::string buf{"Hello, world!"};
  size_t written = 0;
  auto err = anon.write(buf.c_str(), buf.length(), written);
  ASSERT_EQ(p7r::ERR_SUCCESS, err);
  ASSERT_EQ(written, buf.length());

  char rb[200] = { 0 };
  size_t did_read = 0;
  err = fd.read(rb, sizeof(rb), did_read);
  ASSERT_EQ(p7r::ERR_SUCCESS, err);
  ASSERT_EQ(did_read, written);

  std::string res{rb, rb + did_read};
  ASSERT_EQ("Hello, world!", res);
}

// TODO test with wrapping a pipe descriptor; we should
// just be able to use them interchangeably
