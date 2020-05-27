#include <iostream>
#include <string>

#include <packeteer.h>
#include <packeteer/util/url.h>
#include <packeteer/scheduler.h>
#include <packeteer/connector.h>

namespace p7r = packeteer;

#define BUFSIZE 8192


p7r::error_t
echo_callback_stream(p7r::time_point const &, p7r::events_t mask, p7r::error_t, p7r::connector * conn, void *)
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

  // Echo right back.
  size_t written = 0;
  err = conn->write(buf, read, written);
  if (err == p7r::ERR_ASYNC) {
    return p7r::ERR_SUCCESS;
  }
  return err;
}



p7r::error_t
echo_callback_dgram(p7r::time_point const &, p7r::events_t mask, p7r::error_t, p7r::connector * conn, void *)
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

  // Echo right back.
  size_t written = 0;
  err = conn->send(buf, read, written, sender);
  if (err == p7r::ERR_ASYNC) {
    return p7r::ERR_SUCCESS;
  }
  return err;
}



int main(int argc, char **argv)
{
  if (argc < 2) {
    std::cerr << "Usage: echoserver listen-url" << std::endl;
    exit(1);
  }

  try {
    // We can just parse the string as a connect URL; if this fails, nothing else
    // in this server makes much sense.
    auto surl = p7r::util::url::parse(argv[1]);
    std::cout << "Listen URL is: " << surl << std::endl;

    // We need to create an API instance for initializing packeteer. It's passed
    // to the scheduler and connector classes to ensure that while one of those
    // is active, the API stays initialized.
    // It is not recommended to create multiple API instances, because when one
    // goes out of scope, packeteer gets deinitialized.
    auto api = p7r::api::create();

    // Create scheduler - this is for sending I/O to callbacks, instead of having
    // to write a more complex run loop. By default, it creates a number of
    // worker threads, but you can parametrize it to your liking.
    auto scheduler = p7r::scheduler{api};

    // Creating a server connector does not very much yet.
    auto server = p7r::connector{api, surl};

    // Now we have to listen on the connector so that incoming connections or
    // datagrams result in the callback being invoked.
    auto err = server.listen();
    if (err != p7r::ERR_SUCCESS && err != p7r::ERR_ASYNC) {
      std::cerr << p7r::error_name(err) << " // " << p7r::error_message(err) << std::endl;
      return err;
    }

    // Register a connect callback. When a connection comes in, we can accept()
    // it if it's stream-based. If it's datagram-based, we just echo immediately.
    scheduler.register_connector(p7r::PEV_IO_READ, server,
        [&scheduler](p7r::time_point const & now, p7r::events_t mask, p7r::error_t error, p7r::connector * conn, void * baton) -> p7r::error_t
        {
          if (!(mask & p7r::PEV_IO_READ)) {
            return p7r::ERR_SUCCESS;
          }

          // If this connector is stream based, we'll accept() and schedule a new
          // callback for the resulting connector.
          if (conn->get_options() & p7r::CO_STREAM) {
            try {
              auto new_conn = conn->accept();
              if (new_conn.communicating()) {
                std::cout << "Incoming connection accepted." << std::endl;
                scheduler.register_connector(p7r::PEV_IO_READ, new_conn, echo_callback_stream);
              }
              return p7r::ERR_SUCCESS;
            } catch (p7r::exception const & ex) {
              if (ex.code() == p7r::ERR_REPEAT_ACTION) {
                return p7r::ERR_SUCCESS;
              }
              return ex.code();
            }
          }

          // Otherwise call the echo callback.
          return echo_callback_dgram(now, mask, error, conn, baton);
        }
    );

    // If all goes well, loop - we'll do this simply by reading on standard input
    // until CTRL+C is pressed.
    std::string line;
    while (std::getline(std::cin, line)) {
      // Do nothing
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
