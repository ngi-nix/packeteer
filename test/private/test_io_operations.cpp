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
#include "../../lib/connector/win32/io_operations.h"
#include "../../lib/win32/sys_handle.h"

#include <gtest/gtest.h>

#include "../test_name.h"

#include "../../lib/connector/win32/pipe_operations.h"
#include "../../lib/connector/win32/overlapped.h"

#include <thread>
#include <chrono>

namespace p7r = packeteer;
namespace pd = p7r::detail;
namespace io = p7r::detail::io;
namespace sc = std::chrono;

namespace {

struct pipe_context
{
  p7r::handle server;
  p7r::handle client;

  pipe_context(std::string const & name, bool blocking)
  {
    server = pd::create_named_pipe(name, blocking, false);
    EXPECT_TRUE(server.valid());
  
    auto err = pd::connect_to_pipe(client, name, blocking, false);
    EXPECT_EQ(p7r::ERR_SUCCESS, err);
    EXPECT_TRUE(client.valid());
  
    err = pd::poll_for_connection(server);
    EXPECT_EQ(p7r::ERR_SUCCESS, err);
  }
  

  ~pipe_context()
  {
    DisconnectNamedPipe(server.sys_handle()->handle);
    CloseHandle(server.sys_handle()->handle);
    server.sys_handle()->handle = INVALID_HANDLE_VALUE;

    CloseHandle(client.sys_handle()->handle);
    client.sys_handle()->handle = INVALID_HANDLE_VALUE;
  }
};


// Read on first handle, write on second.
inline auto
read_write_non_blocking(
    p7r::handle & first, 
    p7r::handle & second
)
{
  char buf[200] = { 0 };
  ssize_t read = 0;
  auto err = io::read(first,
      buf, sizeof(buf), read);
  ASSERT_EQ(p7r::ERR_REPEAT_ACTION, err);
  ASSERT_EQ(-1, read);

  // A subsequent call should return the same.
  read = 0;
  err = io::read(first,
      buf, sizeof(buf), read);
  ASSERT_EQ(p7r::ERR_REPEAT_ACTION, err);
  ASSERT_EQ(-1, read);

  // If we write to the server end, the next read (probably!) should yield some
  // results.
  std::string msg = "Hello, world!";
  ssize_t written = -1;
  err = io::write(second,
      msg.c_str(), msg.length(), written);
  ASSERT_EQ(p7r::ERR_SUCCESS, err);
  ASSERT_EQ(msg.length(), written);

  // If it's written, we should be able to read it.
  read = 0;
  err = io::read(first,
      buf, sizeof(buf), read);
  ASSERT_EQ(p7r::ERR_SUCCESS, err);
  ASSERT_EQ(msg.length(), read);
}


// Read on first handle, write on second.
inline auto
read_write_blocking(
    p7r::handle & first, 
    p7r::handle & second
)
{
  // Launch a blocking writer thread, and a blocking reader thread. They must
  // both finish, and deliver the result we expect.
  std::string msg = "Hello, world!";
  std::string result;

  // Reader
  std::thread reader([msg, &result, &first]()
  {
    char buf[200] = { 0 };
    ssize_t read = 0;
    auto err = io::read(first,
        buf, sizeof(buf), read);
    ASSERT_EQ(p7r::ERR_SUCCESS, err);
    ASSERT_EQ(msg.length(), read);

    // Copy result
    result = std::string{buf, buf + read};
  });

  // Ensure read can enter
  std::this_thread::sleep_for(sc::milliseconds(50));

  // Writer
  std::thread writer([msg, &second]() 
  {
    ssize_t written = -1;
    auto err = io::write(second,
        msg.c_str(), msg.length(), written);
    ASSERT_EQ(p7r::ERR_SUCCESS, err);
    ASSERT_EQ(msg.length(), written);
  });

  // Join both
  writer.join();
  reader.join();

  // Test result and msg are equal
  ASSERT_EQ(msg, result);
}

 
} // anonymous namespace



TEST(IOOperations, read_client_write_server_non_blocking)
{
  pipe_context ctx{"read_client_write_server_non_blocking", false};
  read_write_non_blocking(ctx.client, ctx.server);
}


TEST(IOOperations, read_server_write_client_non_blocking)
{
  pipe_context ctx{"read_server_write_client_non_blocking", false};
  read_write_non_blocking(ctx.server, ctx.client);
}


TEST(IOOperations, read_client_write_server_blocking)
{
  pipe_context ctx{"read_client_write_server_blocking", true};
  read_write_blocking(ctx.client, ctx.server);
}


TEST(IOOperations, read_server_write_client_blocking)
{
  pipe_context ctx{"read_server_write_client_blocking", true};
  read_write_blocking(ctx.server, ctx.client);
}
