# Examples #

- `echoserver` contains a simple server that echoes every message it gets
  back to the sender. In ~100 lines of code, it can listen on any type of
  connector, stream or datagram based, and handle errors.
- `echoclient` is the server's counterpart, also numbering ~100 lines of
  code. It likewise can connect with any connector. It keeps sending
  messages to its connect URL that you type into stdin.
