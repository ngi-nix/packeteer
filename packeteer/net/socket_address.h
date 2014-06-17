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

#ifndef PACKETEER_NET_SOCKET_ADDRESS_H
#define PACKETEER_NET_SOCKET_ADDRESS_H

// *** Config
#include <packeteer/packeteer.h>
#include <packeteer/net/detail/sysincludes.h>

// *** C includes
#include <sys/socket.h>

// *** C++ includes
#include <iostream>
#include <string>
#include <stdexcept>

namespace packeteer {
namespace net {

/*****************************************************************************
 * Forward declarations
 **/
class network;


/*****************************************************************************
 * Helper defintion
 **/
namespace detail {
/**
 * Union for avoiding casts and breaking aliasing rules. We know by definition
 * that sockaddr_storage is the largest of these.
 **/
union address_type
{
  sockaddr          sa;
  sockaddr_in       sa_in;
  sockaddr_in6      sa_in6;
  sockaddr_storage  sa_storage;
};

} // namespace detail


/*****************************************************************************
 * Socket address
 **/
/**
 * Make it possible to use struct sockaddr as a map key.
 **/
class socket_address
{
public:
  /**
   * Default constructor. The resulting socket address does not point anywhere.
   **/
  socket_address();

  /**
   * Constructor. The 'buf' parameter is expected to be a struct sockaddr of
   * the given length.
   **/
  socket_address(void const * buf, socklen_t len);


  /**
   * Alternative constructor. The string is expected to be a network address
   * in CIDR notation (without the netmask).
   *
   * Throws exception if parsing fails.
   **/
  socket_address(std::string const & address, uint16_t port = 0);
  socket_address(char const * address, uint16_t port = 0);


  /**
   * Verifies the given address string would create a valid socket address.
   **/
  static bool verify_address(std::string const & address);


  /**
   * Verifies that the given netmask would work for the given socket address.
   * This really tests whether the socket address is an IPv4 or IPv6 address,
   * and whether the netmask fits that.
   **/
  bool verify_netmask(size_t const & netmask) const;


  /**
   * Return a CIDR-style string representation of this address (minus port).
   **/
  std::string cidr_str() const;


  /**
   * Returns the port part of this address.
   **/
  uint16_t port() const;


  /**
   * Return a full string representation including port.
   **/
  std::string full_str() const;


  /**
   * Returns the size of the raw address buffer.
   **/
  socklen_t bufsize() const;


  /**
   * Returns the raw address buffer.
   **/
  void const * buffer() const;


  /**
   * Sets/overwrites the port used for this socket address.
   **/
  void set_port(uint16_t port);


  /**
   * Comparison operator is required for map keys. We only overload what's
   * used here, though.
   **/
  bool operator==(socket_address const & other) const;
  bool operator<(socket_address const & other) const;
  bool operator<=(socket_address const & other) const;
  bool operator!=(socket_address const & other) const;

  /**
   * Increment. Returns the address + 1, e.g. 192.168.0.2 if the address is
   * 192.168.0.1.
   *
   * This function does not care about overflows.
   **/
  void operator++();


  /**
   * Return a unique hash of this socket address.
   **/
  size_t hash() const;

private:
  detail::address_type data;

  friend std::ostream & operator<<(std::ostream & os, socket_address const & addr);
  friend class network;
};


/**
 * Formats a socket_address into human-readable form.
 **/
std::ostream & operator<<(std::ostream & os, socket_address const & addr);


}} // namespace packeteer::net


/*******************************************************************************
 * std namespace specializations
 **/
namespace std {

template <> struct hash<packeteer::net::socket_address>
{
  size_t operator()(packeteer::net::socket_address const & x) const
  {
    return x.hash();
  }
};

template <> struct equal_to<packeteer::net::socket_address>
{
  bool operator()(packeteer::net::socket_address const & x, packeteer::net::socket_address const & y) const
  {
    return x.operator==(y);
  }
};

} // namespace std


#endif // guard
