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
    CT_TCP4 = 0,
    CT_TCP6,
    CT_TCP,
    CT_UDP4,
    CT_UDP6,
    CT_UDP,
    CT_FILE,
    CT_IPC,
    CT_PIPE,
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
   *  - file: file system operations (limited usage)
   *  - ipc: UNIX domain sockets/inter-process communication socket;
   *      or named pipe on Windows.
   *  - pipe: UNIX named pipe (unidirectional; not available on Windows)
   *
   * Of these, the first six expect the address string to have the format:
   *    scheme://address[:port]
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

private:
  // pimpl
  struct connector_impl;
  connector_impl *  m_impl;
};

} // namespace packeteer

#endif // guard
