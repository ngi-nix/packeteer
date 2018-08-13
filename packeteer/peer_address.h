/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2017 Jens Finkhaeuser.
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

#include <packeteer/connector.h>
#include <packeteer/net/socket_address.h>

namespace packeteer {

/**
 * The peer_address extends net::socket_address with a connector type. In
 * this way, we can distinguish e.g. UDP and TCP peers with the same IP and
 * port.
 **/

class peer_address
  : public ::packeteer::net::socket_address
  //, public ::packeteer::detail::operators<peer_address>
{
public:
  /**
   * Default constructor. The resulting socket address does not point anywhere.
   **/
  peer_address(connector::connector_type const & type = connector::CT_UNSPEC);

  /**
   * Destructor
   **/
  virtual ~peer_address() = default;

  /**
   * Constructor. The 'buf' parameter is expected to be a struct sockaddr of
   * the given length.
   **/
  peer_address(connector::connector_type const & type, void const * buf, socklen_t len);


  /**
   * Alternative constructor. The string is expected to be a network address
   * in CIDR notation (without the netmask).
   *
   * Throws exception if parsing fails.
   **/
  peer_address(connector::connector_type const & type, std::string const & address,
      uint16_t port = 0);
  peer_address(connector::connector_type const & type, char const * address,
      uint16_t port = 0);
  peer_address(connector::connector_type const & type,
      ::packeteer::net::socket_address const & address);


  /**
   * Constructor from connection string; no separate type or port is necessary
   * as both are included. See the connector class for a description of the
   * string format.
   **/
  explicit peer_address(std::string const & address);


  /**
   * Return the address' connector type.
   **/
  connector::connector_type connector_type() const;


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
   * Behave like a value type.
   **/
  peer_address(peer_address const &) = default;
  peer_address(peer_address &&) = default;
  peer_address & operator=(peer_address const &) = default;

  void swap(peer_address & other);
  size_t hash() const;

protected:
  /**
   * Used by detail::operators
   **/
  //friend class packeteer::detail::operators<peer_address>;

  virtual bool is_equal_to(peer_address const & other) const;
  virtual bool is_less_than(peer_address const & other) const;

private:
  connector::connector_type m_connector_type;
};

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
