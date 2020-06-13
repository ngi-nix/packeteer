/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2014 Unwesen Ltd.
 * Copyright (c) 2015-2020 Jens Finkhaeuser.
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
#include <packeteer.h>

#if defined(PACKETEER_IS_BUILDING)
#include <net/netincludes.h>
#endif // PACKETEER_IS_BUILDING

// *** C++ includes
#include <iostream>
#include <string>
#include <stdexcept>
#include <cstddef>

// *** Own includes
#include <packeteer/error.h>
#include <packeteer/util/operators.h>
#include <packeteer/net/address_type.h>

namespace packeteer::net {

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
union address_data
{
#if defined(PACKETEER_IS_BUILDING)
  sockaddr          sa;
  sockaddr_in       sa_in;
  sockaddr_in6      sa_in6;
  sockaddr_storage  sa_storage;
#if defined(PACKETEER_HAVE_SOCKADDR_UN)
  sockaddr_un       sa_un;
#endif
#endif // PACKETEER_IS_BUILDING
  // For the padding/dummy, see e.g.
  // https://docs.microsoft.com/en-us/previous-versions/windows/desktop/legacy/ms740504(v=vs.85)
  std::byte         _dummy[128];
};

} // namespace detail


/*****************************************************************************
 * Socket address
 **/
/**
 * Make it possible to use struct sockaddr as a map key.
 **/
class PACKETEER_API socket_address
  : public ::packeteer::util::operators<socket_address>
{
public:

  /**
   * Default constructor. The resulting socket address does not point anywhere.
   **/
  socket_address();

  /**
   * Destructor
   **/
  virtual ~socket_address() = default;

  /**
   * Constructor. The 'buf' parameter is expected to be a struct sockaddr of
   * the given length.
   **/
  socket_address(void const * buf, size_t len);


  /**
   * Alternative constructor. The string is expected to be a network address
   * in CIDR notation (without the netmask).
   *
   * Throws exception if parsing fails.
   **/
  explicit socket_address(std::string const & address, uint16_t port = 0);
  explicit socket_address(char const * address, uint16_t port = 0, size_t size = 0);


  /**
   * Verifies the given address string would create a valid IP socket address.
   **/
  static bool verify_cidr(std::string const & address);


  /**
   * Verifies that the given netmask would work for the given socket address.
   * This really tests whether the socket address is an IPv4 or IPv6 address,
   * and whether the netmask fits that.
   **/
  bool verify_netmask(size_t const & netmask) const;


  /**
   * Return a CIDR-style string representation of this address (minus port).
   * Only applicable to IP addresses.
   **/
  std::string cidr_str() const;


  /**
   * Returns the port part of this address.
   * Only applicable to IP addresses.
   **/
  uint16_t port() const;


  /**
   * Returns the socket address type
   **/
  address_type type() const;


  /**
   * Return a full string representation including port.
   **/
  std::string full_str() const;


  /**
   * Returns the size of the raw address buffer.
   **/
  size_t bufsize() const;

  /**
   * Returns the available size of the raw address buffer.
   **/
  size_t bufsize_available() const;


  /**
   * Returns the raw address buffer.
   **/
  void const * buffer() const;
  void * buffer();


  /**
   * Sets/overwrites the port used for this socket address. Returns
   * Returns ERR_INVALID_OPTION if used on the wrong (AT_LOCAL) socket
   * address type.
   **/
  packeteer::error_t set_port(uint16_t port);


  /**
   * Behave like a value type.
   **/
  socket_address(socket_address const &) = default;
  socket_address(socket_address &&) = default;
  socket_address & operator=(socket_address const &) = default;

  void swap(socket_address & other);
  size_t hash() const;

  /**
   * Increment. Returns the address + 1, e.g. 192.168.0.2 if the address is
   * 192.168.0.1.
   *
   * This function does not care about overflows.
   **/
  void operator++();

  /**
   * Used by util::operators
   **/
  bool is_equal_to(socket_address const & other) const;
  bool is_less_than(socket_address const & other) const;


private:
  detail::address_data data;

  friend PACKETEER_API_FRIEND std::ostream & operator<<(std::ostream & os, socket_address const & addr);
  friend class PACKETEER_API_FRIEND network;
};


/**
 * Formats a socket_address into human-readable form.
 **/
PACKETEER_API std::ostream & operator<<(std::ostream & os, socket_address const & addr);

/**
 * Swappable
 **/
inline void
swap(socket_address & first, socket_address & second)
{
  return first.swap(second);
}

} // namespace packeteer::net


/*******************************************************************************
 * std namespace specializations
 **/
namespace std {

template <> struct PACKETEER_API hash<packeteer::net::socket_address>
{
  size_t operator()(packeteer::net::socket_address const & x) const
  {
    return x.hash();
  }
};


} // namespace std


#endif // guard
