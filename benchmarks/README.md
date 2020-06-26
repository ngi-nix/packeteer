Benchmarks
==========

Benchmarks are not guaranteed to build on any platform, or build free of
warnings. They're deliberately kept simple.

Benchmarks try to compare packeteer to competitors in various scenarios. As
a result, they're split into a generic part implementing the benchmark
algorithm, and backend-specific implementation files.

1. `event` - an event cascading benchmark, based on `libevent`'s benchmark. It
  benchmarks the responsiveness to I/O events by:
  - Opening a number of UDP sockets.
  - Writing a byte on a subset of the sockets, always sending to the next socket.
  - When a byte is received, a byte is written to the next socket in line.
  - When all sockets have been exercised in this way, the test terminates and
    wall time used is output.

  The overall effect is to measure *wall* time for this cascade to complete.
  As responsiveness to I/O events is a major contributing factor to this,
  it indirectly measures responsiveness.
1. See https://gitlab.com/interpeer/packeteer/-/issues/23
