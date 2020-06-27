Overview
========

Packeteer provides a cross-platform socket API in C++.

The aim of the project is to get high performance, asynchronous inter-process
I/O into any project without the need for a ton of external libraries, or for
writing cross-platform boilerplate.

Packeteer achieves this aim by:
- Picking the best available event notification for the target system.
- and providing a socket-like API for supported IPC.

The project was started a long time ago, but was not public until recently.
Some of the code has been extensively used in commercial products.


Quick Start
===========

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


Building
========

Requirements
------------

- [meson](https://mesonbuild.com/) for the build system. Meson is most
  easily installed via [python](https://www.python.org/)'s `pip` tool.
  Installing Python als means you can use the `android.py` script for Android
  builds.
- [ninja](https://ninja-build.org/) or platform-specific tools for build
  execution.
- Packeteer is implemented in C++, and requires some compiler support for
  the C++17 standard.
- Depending on which scheduler implementation you want to use, packeteer may
  require specific OS and kernel versions, e.g. Linux 2.6.9+ for the epoll
  scheduler, etc.

If you're running a recent-ish UNIX-derivative, it's probably best to just
download packeteer and try to compile it.

Instructions
------------

1. `pip install meson`
1. `mkdir build && cd build`
1. `meson ..`
1. `ninja`

Instructions (Android)
----------------------

1. `pip install meson pipenv`
1. `pipenv install`
1. `pipenv run ./android.py`
1. `mkdir build && cd build`
1. `meson --cross-file ../android-<target>.txt ..`
1. `ninja`

Replace `<target>` with the specific Android target you wish to build for.

Scope
=====

Design Goals
------------

1. Cross platform (highest priority)
1. Low usage complexity
1. Stable API/ABI
1. Packet oriented I/O friendly
1. Scalable
1. Efficient
1. Extensible (lowest priority)

The ordering of the goals is somewhat important. If you're just looking for
fast event notification, there are better projects out there. If you're just
looking for TCP streams, there are better projects, etc.

The overarching goal is to have very little effort in developing client and
server implementations for new low-level protocols.


Supported Connectors
--------------------


- TCP/IP via the "tcp", "tcp4" and "tcp6" schemes.
- UDP/IP via the "udp", "udp4" and "udp6" schemes.
- `AF_LOCAL/AF_UNIX` via the "local" scheme. Note that unnamed local sockets are
  emulated on Windows, and abstract local names are not supported on all
  platforms.
- Windows named pipes via the "pipe" scheme.
- POSIX named pipes via the "fifo" scheme.
- The "anon" scheme maps to POSIX anonymous pipes, or Windows named pipes
  with a generated name, and is usable for IPC within a process.


License
=======

See the COPYING file.
