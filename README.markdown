# Overview #
Packetflinger provides a simplified, asynchronous, event-based socket API.

By itself, it is not a full-featured networking library, but implements common
boilerplate for high-performance networking. In particular, it provides the
following:

- A simple server abstraction for listening on a particular type of socket.
- A simple client (or rather connection) abstraction for connecting to a server.
- A dumb-as-it-can-get non-blocking write function for connections. Write data
  to it, and the backend takes care of sending it over the connection as soon
  as it can.
- A primitive thread pool that schedules event callbacks such as the fact that
  data has been read on a connection on as many or as few threads as you want.
  There is very deliberately no logic to automagically shrink or grow the thread
  pool, but there is functionality that would allow you to implement such thread
  pool management on top of packetflinger.
- A highly performant main loop for all of this logic, running in a scheduler
  thread. There is a choice of implementations for this, based on epoll, kqueue
  or select. It is expected that the choice of implementations will expand
  in future to I/O completion ports on Windows systems.

What packetflinger is not, amongst other things, is a message queue system. For
one thing, there is no concept of messages and all that implies. For another,
there is not much queueing going on.

It just flings raw data packets. Fast.

# Requirements #

- Packetflinger is implemented in C++, and requires some compiler support for
  the new C++11 standard. If you're using GCC 4.6 and above, things should work
  for you.
- Packetflinger uses some [boost](http://www.boost.org/) libraries, including
  the threading library.
- Depending on which scheduler implementation you want to use, packetflinger may
  require specific OS and kernel versions, e.g. Linux 2.6.9+ for the epoll
  scheduler, etc.

If you're running a recent-ish UNIX-derivative, it's probably best to just
download packetflinger and try to compile it.

# License #

See the COPYING file.
