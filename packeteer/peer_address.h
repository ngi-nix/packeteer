/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2017-2019 Jens Finkhaeuser.
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
#ifndef PACKETEER_PEER_ADDRESS_H
#define PACKETEER_PEER_ADDRESS_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer/packeteer.h>

#include <packeteer/connector_specs.h>
#include <packeteer/net/socket_address.h>
#include <packeteer/util/url.h>
#include <packeteer/util/operators.h>

namespace packeteer {

/**
 * The peer_address extends net::socket_address with a connector type. In
 * this way, we can distinguish e.g. UDP and TCP peers with the same IP and
 * port.
 **/

class peer_address
  : public ::packeteer::util::operators<peer_address>
{
public:
  /**
   * Default constructor. The resulting socket address does not point anywhere.
   **/
  peer_address(connector_type const & type = CT_UNSPEC);

  /**
   * Destructor
   **/
  virtual ~peer_address() = default;

  /**
   * Constructor. The 'buf' parameter is expected to be a struct sockaddr of
   * the given length.
   **/
  peer_address(connector_type const & type, void const * buf, socklen_t len);


  /**
   * Alternative constructor. The string is expected to be a network address
   * in CIDR notation (without the netmask).
   *
   * Throws exception if parsing fails.
   **/
  peer_address(connector_type const & type, std::string const & address,
      uint16_t port = 0);
  peer_address(connector_type const & type, char const * address,
      uint16_t port = 0);
  peer_address(connector_type const & type,
      ::packeteer::net::socket_address const & address);


  /**
   * Constructor from connection string or url; no separate type or port is
   * necessary as both are included. See the connector class for a description
   * of the string format.
   **/
  explicit peer_address(std::string const & address);
  explicit peer_address(::packeteer::util::url const & url);


  /**
   * Return the address' connector type.
   **/
  connector_type & conn_type();
  connector_type const & conn_type() const;


  /**
   * Return the scheme for this peer address.
   **/
  std::string scheme() const;


  /**
   * Return a full string representation of this peer_address such that it
   * can be handed to the appropriate constructor (above) and generate an
   * equal peer_address.
   **/
  std::string str() const;

  /**
   * Expose the socket address, too.
   **/
  net::socket_address & socket_address();
  net::socket_address const & socket_address() const;

  /**
   * Behave like a value type.
   **/
  peer_address(peer_address const &) = default;
  peer_address(peer_address &&) = default;
  peer_address & operator=(peer_address const &) = default;

  void swap(peer_address & other);
  size_t hash() const;

  /**
   * Used by util::operators
   **/

  bool is_equal_to(peer_address const & other) const;
  bool is_less_than(peer_address const & other) const;

private:
  net::socket_address m_sockaddr;
  connector_type      m_connector_type;

  friend std::ostream & operator<<(std::ostream & os, peer_address const & addr);
};


/**
 * Formats a peer_address into human-readable form.
 **/
std::ostream & operator<<(std::ostream & os, peer_address const & addr);


} // namespace packeteer

/*******************************************************************************
 * std namespace specializations
 **/
namespace std {

template <> struct hash<packeteer::peer_address>
{
  size_t operator()(packeteer::peer_address const & x) const
  {
    return x.hash();
  }
};

template <>
inline void
swap<::packeteer::peer_address>(::packeteer::peer_address & first, ::packeteer::peer_address & second)
{
  return first.swap(second);
}

} // namespace std

#endif // guard
