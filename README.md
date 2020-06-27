# Overview #

Packeteer provides a cross-platform socket API in C++.

The aim of the project is to get high performance, asynchronous inter-process
I/O into any project without the need for a ton of external libraries, or for
writing cross-platform boilerplate.

Packeteer achieves this aim by:
- Picking the best available event notification for the target system.
- and providing a socket-like API for supported IPC.

The project was started a long time ago, but was not public until recently.
Some of the code has been extensively used in commercial products.

# Quick Start #

You can browse the `examples` folder for an echo server and client
implementation, each in ~100 lines of code. The gist, however, is this:

1. Create a `connector`, which provides the socket-like API:
   ```c++
   auto conn = packeteer::connector{api, "tcp://192.168.0.1:4321"};
   auto err = conn.connect();
   ```
1. If a server was listening on the given address and port, you can
   now start with I/O:
   ```c++
   size_t transferred = 0;
   err = conn.write(buf, buflen, transferred);
   err = conn.read(buf, buflen, transferred);
   ```
   This, of course, can fail if there is no data to transfer.
1. Asynchronous I/O is achieved by registering callbacks for I/O events
   with a scheduler object.
   ```c++
   using namespace packeteer;
   auto sched = scheduler{api};
   sched.register_connector(PEV_IO_READ, conn,
       [*](time_point const & now, events_t events, connector * the_conn) -> error_t
       {
          // Is notified when the given connector is readable.
          the_conn->read(buf, buflen, transferred);
       }
    );
   ```
1. The rest of the echoserver and echoclient implementations are taken up
   by asynchronously accepting connections and error handling.


# Supported Connectors #

- TCP/IP via the "tcp", "tcp4" and "tcp6" schemes.
- UDP/IP via the "udp", "udp4" and "udp6" schemes.
- `AF_LOCAL/AF_UNIX` via the "local" scheme. Note that unnamed local sockets are
  emulated on Windows, and abstract local names are not supported on all
  platforms.
- Windows named pipes via the "pipe" scheme.
- POSIX named pipes via the "fifo" scheme.
- The "anon" scheme maps to POSIX anonymous pipes, or Windows named pipes
  with a generated name, and is usable for IPC within a process.


# Requirements #

- Packeteer is implemented in C++, and requires some compiler support for
  the C++17 standard.
- Depending on which scheduler implementation you want to use, packeteer may
  require specific OS and kernel versions, e.g. Linux 2.6.9+ for the epoll
  scheduler, etc.

If you're running a recent-ish UNIX-derivative, it's probably best to just
download packeteer and try to compile it.

# License #

See the COPYING file.
