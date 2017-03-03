/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2014 Unwesen Ltd.
 * Copyright (c) 2015-2017 Jens Finkhaeuser.
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

#include <algorithm>
#include <utility>
#include <functional>

#include <packeteer/handle.h>
#include <packeteer/error.h>

#include <packeteer/net/socket_address.h>

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
    CT_LOCAL,
    CT_PIPE,
    CT_ANON,
  };

  // Connector behaviour
  enum connector_behaviour
  {
    CB_DEFAULT  = 0,  // typically the best pick
    CB_STREAM   = 1,  // STREAM connector; use read()/write()
    CB_DATAGRAM = 2,  // DATAGRAM connector; use receive()/send()
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
   *  - local: UNIX domain sockets (POSIX)
   *  - pipe: UNIX named pipe or Windows named pipe
   *
   * Of these, the first six expect the address string to have the format:
   *    scheme://address[:port]
   *
   * FIXME this keyword scheme seems weird; instead, non-blocking should be the
   * default, and blocking should be something that can be set on all connectors.
   *
   * The anonymous pipe expects the scheme to be followed by nothing at all,
   * or one of the keywords "blocking" or "nonblocking", to specify whether
   * a blocking or non-blocking pipe is expected. If the keyword is left out,
   * a non-blocking pipe is created.
   *    anon://[blocking|nonblocking]
   *
   * The last few expect the following format:
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
  connector(std::string const & address,
      connector_behaviour const & behaviour = CB_DEFAULT);
  ~connector();

  /**
   * Returns the connector type.
   **/
  connector_type type() const;

  /**
   * Returns the connector's address.
   **/
  std::string address() const;

  /**
   * In the spirit of socket-style APIs, a connector that is listening on
   * its address can be used as the server side of a network communications
   * channel. That is, once listening on the specified address, other parties
   * can connect to it.
   *
   * Listening to an anon-URI automatically also connects the connector.
   *
   * Returns an error if listening fails.
   *
   * Note: This function is not a direct mapping to POSIX listen(). Instead
   *       it is a combination of bind() and listen(), and it's exact
   *       behaviour depends on the protocol chosen. For UDP, for example,
   *       no POSIX listen() call is issued.
   **/
  error_t listen();
  bool listening() const;

  /**
   * Similarly, connecting to the specified address creates a connector
   * usable for the client side of a network communications channel.
   *
   * Connecting to URIs establishes a connection to the endpoint specified in
   * the URI.
   *
   * Connecting to an anon-URI automatically also turns the connector to
   * listening.
   *
   * Returns an error if connecting fails.
   **/
  error_t connect();
  bool connected() const;

  /**
   * If a listening socket receives a connection request, you can accept this
   * via the accept() function. The function returns a new connector of the
   * same type with the connected() flag set (see above).
   *
   * Some types of connectors may return a shallow copy of themselves. In that
   * case, the read and write handles are the same as the connector's whose
   * accept() function was called, but this is intentional. Similarly, the
   * return value of address(), etc. should be identical.
   *
   * Other connectors may return a new connector, but still retain the same
   * return value for address().
   *
   * The API favours a usage pattern where no matter the underlying connector
   * implementation, a connector returned by accept() from a server connector
   * is valid for I/O with a connected client connector.
   **/
  connector accept() const;

  /**
   * Connectors, once bound or connected, have one or more file descriptors
   * associated with them. Normally, there is one file descriptor for reading,
   * and one file descriptor for writing.
   *
   * You typically want to use these file descriptors together with the
   * scheduler class.
   *
   * Return an invalid handle if the connector is neither bound nor connected.
   **/
  handle get_read_handle() const;
  handle get_write_handle() const;

  /**
   * Read from and write messages to a given peer address on the connector. The
   * semantics are effectively identical to the POSIX functions of similar name.
   *
   * Note that receive() and send() are best used for CB_DATAGRAM connectors.
   **/
  error_t receive(void * buf, size_t bufsize, size_t & bytes_read,
      ::packeteer::net::socket_address & sender);
  error_t send(void const * buf, size_t bufsize, size_t & bytes_written,
      ::packeteer::net::socket_address const & recipient);

  /**
   * Peek how much data is available to receive(). This is best use for
   * CB_DATAGRAM connectors, when you're unsure about the datagram size.
   *
   * Of course use of this function translates to another system call, so it's
   * best used sparingly, e.g. for determining the best datagram size for a new
   * connector.
   **/
  size_t peek() const;

  /**
   * Read from and write to the connector.
   *
   * read(): read at most bufsize bytes into the buffer pointed to by buf. The
   *    actual amount of bytes read is returned in the bytes_read parameter.
   * write(): write bufsize bytes from buf into the connector. Return the actual
   *    amount written in the bytes_written parameter.
   *
   * Return errors if the connector is neither bound nor connected.
   *
   * Note that read() and write() are best used for CB_STREAM connectors.
   **/
  error_t read(void * buf, size_t bufsize, size_t & bytes_read);
  error_t write(void const * buf, size_t bufsize, size_t & bytes_written);

  /**
   * Close the connector, making it neither bound nor connected. Subsequent
   * calls to listen() or connect() should be valid again.
   **/
  error_t close();


  /**
   * Behave like a value type.
   **/
  connector(connector const & other);
  connector(connector &&) = default;

  bool operator==(connector const & other) const;
  bool operator<(connector const & other) const;

  void swap(connector & other);
  size_t hash() const;

private:
  connector();

  // pimpl
  struct connector_impl;
  connector_impl * m_impl;
};


} // namespace packeteer

/**
 * std specializations for connector
 **/
namespace std {

template <> struct hash<::packeteer::connector>
{
  inline size_t operator()(::packeteer::connector const & conn)
  {
    return conn.hash();
  }
};


template <>
inline void
swap<::packeteer::connector>(::packeteer::connector & first, ::packeteer::connector & second)
{
  return first.swap(second);
}

} // namespace std



#endif // guard
