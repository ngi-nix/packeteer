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
#include <iostream>
#include <string>

#include <packeteer.h>
#include <packeteer/util/url.h>
#include <packeteer/scheduler.h>
#include <packeteer/connector.h>
#include <packeteer/net/socket_address.h>

namespace p7r = packeteer;

#define BUFSIZE 8192


p7r::error_t
read_callback_stream(p7r::time_point const &, p7r::events_t mask, p7r::error_t, p7r::connector * conn, void *)
{
  if (!(mask & p7r::PEV_IO_READ)) {
    return p7r::ERR_SUCCESS;
  }

  // Read
  char buf[BUFSIZE];
  size_t read = 0;
  auto err = conn->read(buf, sizeof(buf), read);
  if (err != p7r::ERR_SUCCESS) {
    return err;
  }

  // Output
  std::string msg{buf, buf + read};
  std::cout << "Received: " << msg << std::endl;

  return p7r::ERR_SUCCESS;
}



p7r::error_t
read_callback_dgram(p7r::time_point const &, p7r::events_t mask, p7r::error_t, p7r::connector * conn, void *)
{
  if (!(mask & p7r::PEV_IO_READ)) {
    return p7r::ERR_SUCCESS;
  }

  // Read
  char buf[BUFSIZE];
  size_t read = 0;
  p7r::net::socket_address sender;
  auto err = conn->receive(buf, sizeof(buf), read, sender);
  if (err != p7r::ERR_SUCCESS) {
    return err;
  }

  // Output
  std::string msg{buf, buf + read};
  std::cout << "Received from " << sender << ": " << msg << std::endl;

  return p7r::ERR_SUCCESS;
}



int main(int argc, char **argv)
{
  if (argc < 2) {
    std::cerr << "Usage: echoserver connect-url [listen-url]" << std::endl << std::endl;
    std::cerr << "Stream-based connectors need a URL to connect to. Datagram-based" << std::endl;
    std::cerr << "additionally need a URL to listen on." << std::endl;
    exit(1);
  }

  try {
    // We can just parse the string as a connect URL; if this fails, nothing else
    // in this server makes much sense.
    auto curl = p7r::util::url::parse(argv[1]);
    std::cout << "Connect URL is: " << curl << std::endl;

    p7r::net::socket_address caddr;
    p7r::util::url listen_url;
    if (argc > 2) {
      listen_url = p7r::util::url::parse(argv[2]);
      caddr = p7r::net::socket_address{curl.authority};
      std::cout << "Listen URL is: " << listen_url << std::endl;
    }


    // We need to create an API instance and scheduler - see echoserver.
    auto api = p7r::api::create();
    auto scheduler = p7r::scheduler{api};

    // Datagram connections need us to listen(), but stream connections need us
    // to connect().
    auto client = p7r::connector{api, curl};
    p7r::error_t err;
    if (client.get_options() & p7r::CO_STREAM) {
      err = client.connect();
    }
    else {
      client = p7r::connector{api, listen_url};
      err = client.listen();
    }
    if (err != p7r::ERR_SUCCESS && err != p7r::ERR_ASYNC) {
      std::cerr << p7r::error_name(err) << " // " << p7r::error_message(err) << std::endl;
      return err;
    }

    // Register a read callback - it just reads what was sent.
    if (client.get_options() & p7r::CO_STREAM) {
      scheduler.register_connector(p7r::PEV_IO_READ, client, read_callback_stream);
    }
    else {
      scheduler.register_connector(p7r::PEV_IO_READ, client, read_callback_dgram);
    }

    // If all goes well, loop - we'll do this simply by reading on standard input
    // until CTRL+C is pressed.
    std::cout << "Any line you enter is sent to the echo server, except if you type 'exit'." << std::endl;
    std::string line;
    while (std::getline(std::cin, line)) {
      if (line == "exit") {
        break;
      }

      size_t written = 0;
      if (client.get_options() & p7r::CO_STREAM) {
        client.write(line.c_str(), line.size(), written);
      }
      else {
        client.send(line.c_str(), line.size(), written, caddr);
      }
    }

  } catch (p7r::exception const & ex) {
    std::cerr << ex.what() << std::endl;
    return ex.code();
  } catch (std::exception const & ex) {
    std::cerr << ex.what() << std::endl;
    return -1;
  } catch (...) {
    std::cerr << "Unknown error." << std::endl;
    return -2;
  }

  return 0;
}
