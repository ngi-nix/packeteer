/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@unwesen.co.uk>
 *
 * Copyright (c) 2014 Unwesen Ltd.
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
#ifndef PACKETEER_CONNECTOR_H
#define PACKETEER_CONNECTOR_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer/packeteer.h>

#include <packeteer/error.h>

namespace packeteer {

/**
 * The connector class provides a socket-like API for handling I/O.
 *
 * We're deliberately not re-using the socket term, as it is associated with
 * network I/O specifically, whereas packeteer can handle other types of I/O.
 * However, the usage is fairly similar to how you'd use sockets.
 **/
class connector
{
public:
  /***************************************************************************
   * Types
   **/
  // Types of connectors
  enum connector_type
  {
    CT_UNSPEC = -1, // Never returned
    CT_TCP4 = 0,
    CT_TCP6,
    CT_TCP,
    CT_UDP4,
    CT_UDP6,
    CT_UDP,
    CT_FILE,
    CT_IPC,
    CT_PIPE,
    CT_ANON,
  };



  /***************************************************************************
   * Interface
   **/
  /**
   * Constructor. Specify the address the connector represents, as a URI
   * string. Possible schemes are:
   *
   *  - tcp4: IPv4 TCP connection
   *  - tcp6: IPv6 TCP connection
   *  - tcp: IPv4 and/or IPv6 TCP connection, depending on availability
   *  - udp4: IPv4 UDP connection
   *  - udp6: IPv6 UDP connection
   *  - udp: IPv4 or IPv6 UDP connection, depending on availability
   *  - anon: anonymous pipe, similar to the pipe class. Mostly usable for
   *      IPC between processes that inherit the file descriptors. Anonymous
   *      pipes are unidirectional.
   *  - file: file system operations (limited usage)
   *  - ipc: UNIX domain sockets/inter-process communication socket;
   *      or named pipe on Windows.
   *  - pipe: UNIX named pipe (unidirectional; not available on Windows)
   *
   * Of these, the first six expect the address string to have the format:
   *    scheme://address[:port]
   *
   * The anonymous pipe expects the scheme to be followed by nothing at all,
   * i.e. the URI string must always be:
   *    anon://
   *
   * The last three expect the following format:
   *    scheme://path
   *
   * The scheme part is not case sensitive. The address for IPv6 is not case
   * sensitive. Path names are case sensitive on operating systems where path
   * names generally are.
   *
   * The constructor will throw if an address could not be parsed. Note that
   * paths will not be parsed immediately, but only when the connection
   * specified by the connector is to be established.
   **/
  connector(std::string const & address);
  ~connector();

  /**
   * Returns the connector type.
   **/
  connector_type type() const;

  /**
   * In the spirit of socket-style APIs, a connector that is bound to its
   * address can be used as the server side of a network communications
   * channel. That is, once bound to the specified address, other parties
   * can connect to it.
   *
   * Binding to a file-URI does not make sense, and will therefore be
   * rejected.
   *
   * Binding an anon-URI automatically also connects the connector.
   *
   * Returns an error if binding fails.
   **/
  error_t bind();
  bool bound() const;

  /**
   * Similarly, connecting to the specified address creates a connector
   * usable for the client side of a network communications channel.
   *
   * Connecting to a file-URI effectively opens the file for I/O. Connecting
   * to other URIs establishes a connection to the endpoint specified in the
   * URI.
   *
   * Connecting to an anon-URI automatically also binds the connector.
   *
   * Returns an error if connecting fails.
   **/
  error_t connect();
  bool connected() const;

  /**
   * Connectors, once bound or connected, have one or more file descriptors
   * associated with them. Normally, there is one file descriptor for reading,
   * and one file descriptor for writing.
   *
   * You typically want to use these file descriptors together with the
   * scheduler class.
   *
   * Return -1 if the connector is neither bound nor connected.
   **/
  int get_read_fd() const;
  int get_write_fd() const;


  /**
   * Read from and write to the connector.
   *
   * read(): read at most bufsize bytes into the buffer pointed to by buf. The
   *    actual amount of bytes read is returned in the bytes_read parameter.
   * write(): write bufsize bytes from buf into the connector. Return the actual
   *    amount written in the bytes_written parameter.
   *
   * Return errors if the connector is neither bound nor connected.
   **/
  error_t read(void * buf, size_t bufsize, size_t & bytes_read);
  error_t write(void const * buf, size_t bufsize, size_t & bytes_written);


  /**
   * Close the connector, making it neither bound nor connected. Subsequent
   * calls to bind() or connect() should be valid again.
   **/
  error_t close();

private:
  // pimpl
  struct connector_impl;
  connector_impl * m_impl;
};

} // namespace packeteer

#endif // guard
