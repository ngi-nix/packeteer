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

#include <packeteer/net/socket_address.h>

#include <cstring>
#include <sstream>

#include <meta/hash.h>

#include <packeteer/error.h>

#include <packeteer/net/detail/cidr.h>

namespace packeteer {
namespace net {
/*****************************************************************************
 * Helper functions
 **/
namespace {

template <typename T>
char const * unwrap(T const & source)
{
  return source;
}

template<>
char const * unwrap<std::string>(std::string const & source)
{
  return source.c_str();
}


template <typename T>
void
parse_address(detail::address_type & data, T const & source, uint16_t port)
{
  // Need to zero data.
  ::memset(&data.sa_storage, 0, sizeof(data));

  // Try parsing as a CIDR address.
  sa_family_t proto = AF_UNSPEC;
  ssize_t err = detail::parse_extended_cidr(source, true, data, proto,
      port);
  if (0 == err && AF_UNSPEC != proto) {
    // All good.
    return;
  }

  // If that didn't work, we assume the source specifies a path name.
  ::memset(&data.sa_storage, 0, sizeof(data));
  data.sa_un.sun_family = AF_LOCAL;
  ::snprintf(data.sa_un.sun_path, UNIX_PATH_MAX, "%s", unwrap(source));
}

} // anonymous namespace

/*****************************************************************************
 * Member functions
 **/
socket_address::socket_address()
{
  // Need to zero data.
  ::memset(&data.sa_storage, 0, sizeof(data));
}



socket_address::socket_address(void const * buf, socklen_t len)
{
  if (static_cast<size_t>(len) > sizeof(data)) {
    throw exception(ERR_INVALID_VALUE, "socket_address: input buffer too large.");
  }

  // Need to zero data.
  ::memset(&data.sa_storage, 0, sizeof(data));

  if (nullptr != buf) {
    ::memcpy(&data.sa_storage, buf, len);
  }
}



socket_address::socket_address(std::string const & address,
    uint16_t port /* = 0 */)
{
  parse_address(data, address, port);
}



socket_address::socket_address(char const * address, uint16_t port /* = 0 */)
{
  parse_address(data, address, port);
}



bool
socket_address::verify_cidr(std::string const & address)
{
  detail::address_type dummy_addr;
  sa_family_t proto = AF_UNSPEC;
  size_t err = detail::parse_extended_cidr(address, true, dummy_addr,
      proto);
  return (0 == err && AF_UNSPEC != proto);
}



bool
socket_address::verify_netmask(size_t const & netmask) const
{
  if (AF_INET == data.sa_storage.ss_family) {
    return (netmask <= 32);
  }
  else if (AF_INET6 == data.sa_storage.ss_family) {
    return (netmask <= 128);
  }
  else {
    return false;
  }
}



std::string
socket_address::cidr_str() const
{
  // Interpret data as sockaddr_storage, sockaddr_in and sockaddr_in6. Of the
  // latter two, only one is safe to use!
  char buf[INET6_ADDRSTRLEN] = { '\0' };
  if (AF_INET == data.sa_storage.ss_family) {
    inet_ntop(data.sa_storage.ss_family, &(data.sa_in.sin_addr), buf, sizeof(buf));
  }
  else if (AF_INET6 == data.sa_storage.ss_family) {
    inet_ntop(data.sa_storage.ss_family, &(data.sa_in6.sin6_addr), buf, sizeof(buf));
  }

  return buf;
}



uint16_t
socket_address::port() const
{
  // Interpret data as sockaddr_storage, sockaddr_in and sockaddr_in6. Of the
  // latter two, only one is safe to use!
  switch (data.sa_storage.ss_family) {
    case AF_INET:
      return ntohs(data.sa_in.sin_port);

    case AF_INET6:
      return ntohs(data.sa_in6.sin6_port);

    default:
      return 0;
  }
}



std::string
socket_address::full_str() const
{
  std::stringstream s;

  switch (data.sa_storage.ss_family) {
    case AF_INET:
    case AF_INET6:
      s << cidr_str() << ":" << port();
      break;

    case AF_LOCAL:
      s << data.sa_un.sun_path;
      break;

    default:
      break;
  }

  return s.str();
}



socklen_t
socket_address::bufsize() const
{
  switch (data.sa_storage.ss_family) {
    case AF_INET:
      return sizeof(sockaddr_in);

    case AF_INET6:
      return sizeof(sockaddr_in6);

    case AF_LOCAL:
      return sizeof(sockaddr_un);

    default:
      break;
  }
  return 0;
}



void const *
socket_address::buffer() const
{
  return &(data.sa_storage);
}



error_t
socket_address::set_port(uint16_t port)
{
  // Interpret data as sockaddr_storage, sockaddr_in and sockaddr_in6. Of the
  // latter two, only one is safe to use!
  switch (data.sa_storage.ss_family) {
    case AF_INET:
      data.sa_in.sin_port = htons(port);
      break;

    case AF_INET6:
      data.sa_in6.sin6_port = htons(port);
      break;

    default:
      return ERR_INVALID_OPTION;
  }

  return ERR_SUCCESS;
}



bool
socket_address::operator==(socket_address const & other) const
{
  // First compare families. It's a bit hard to decide what to return if the
  // families mismatch, but 'false' seems sensible.
  if (data.sa_storage.ss_family != other.data.sa_storage.ss_family) {
    return false;
  }

  // Next, depending on IPv4 or IPv6, compare the address part of the data.
  void const * compare_buf = nullptr;
  void const * compare_buf_other = nullptr;
  size_t compare_size = 0;

  // Now compare depending on family. IPv4 first
  if (AF_INET == data.sa_storage.ss_family) {
    if (data.sa_in.sin_port != other.data.sa_in.sin_port) {
      return false;
    }

    compare_size = sizeof(in_addr);
    compare_buf = &(data.sa_in.sin_addr);
    compare_buf_other = &(other.data.sa_in.sin_addr);
  }

  // IPv6
  else if (AF_INET6 == data.sa_storage.ss_family) {
    if (data.sa_in6.sin6_port != other.data.sa_in6.sin6_port) {
      return false;
    }

    compare_size = sizeof(in6_addr);
    compare_buf = &(data.sa_in6.sin6_addr);
    compare_buf_other = &(other.data.sa_in6.sin6_addr);
  }

  // UNIX
  else {
    compare_size = UNIX_PATH_MAX;
    compare_buf = data.sa_un.sun_path;
    compare_buf_other = other.data.sa_un.sun_path;
  }

  return 0 == ::memcmp(compare_buf, compare_buf_other, compare_size);
}



bool
socket_address::operator<(socket_address const & other) const
{
  // First compare families. It's a bit hard to decide what to return if the
  // families mismatch, but 'false' seems sensible.
  if (data.sa_storage.ss_family != other.data.sa_storage.ss_family) {
    return false;
  }


  // Now compare depending on family. IPv4 is simple.
  if (AF_INET == data.sa_storage.ss_family) {
    if (ntohl(data.sa_in.sin_addr.s_addr) < ntohl(other.data.sa_in.sin_addr.s_addr)) {
      return true;
    }
    return (ntohs(data.sa_in.sin_port) < ntohs(other.data.sa_in.sin_port));
  }

  // IPv6 is harder, but not too hard. We need to compare byte by byte.
  else if (AF_INET6 == data.sa_storage.ss_family) {
    for (ssize_t i = 0 ; i < 16 ; ++i) {
      if (data.sa_in6.sin6_addr.s6_addr[i] < other.data.sa_in6.sin6_addr.s6_addr[i]) {
        return true;
      }
    }

    return (ntohs(data.sa_in6.sin6_port) < ntohs(other.data.sa_in6.sin6_port));
  }

  // Unix paths are simple again.
  else {
    return 0 > ::memcmp(data.sa_un.sun_path, other.data.sa_un.sun_path, UNIX_PATH_MAX);
  }
}



void
socket_address::operator++()
{
  // As always, IPv4 is pretty simple.
  if (AF_INET == data.sa_storage.ss_family) {
    uint32_t ip = ntohl(data.sa_in.sin_addr.s_addr);
    data.sa_in.sin_addr.s_addr = htonl(++ip);
  }
  else if (AF_INET6) {
    // IPv6 is still simple enough, we just have to handle overflow from one
    // byte into the next.
    bool done = false;
    for (ssize_t i = 15 ; i >= 0 && !done ; --i) {
      if (data.sa_in6.sin6_addr.s6_addr[i] < 0xff) {
        ++data.sa_in6.sin6_addr.s6_addr[i];
        done = true;
      }
    }
  }
  else {
    throw exception(ERR_UNSUPPORTED_ACTION, "Don't know how to increment paths.");
  }
}



socket_address::socket_address_type
socket_address::type() const
{
  switch (data.sa_storage.ss_family) {
    case AF_INET:
      return SAT_INET4;

    case AF_INET6:
      return SAT_INET6;

    case AF_LOCAL:
      return SAT_LOCAL;

    default:
      return SAT_UNSPEC;
  }
}



size_t
socket_address::hash() const
{
  // Figure out buffer to compare.
  void const * hash_buf = nullptr;
  size_t hash_size = 0;

  uint16_t port = 0;

  // IPv4
  if (AF_INET == data.sa_storage.ss_family) {
    hash_size = sizeof(in_addr);
    hash_buf = &(data.sa_in.sin_addr);
    port = data.sa_in.sin_port;
  }

  // IPv6
  else if (AF_INET6 == data.sa_storage.ss_family) {
    hash_size = sizeof(in6_addr);
    hash_buf = &(data.sa_in6.sin6_addr);
    port = data.sa_in6.sin6_port;
  }

  // UNIX
  else {
    hash_size = ::strlen(data.sa_un.sun_path);
    hash_buf = data.sa_un.sun_path;
    port = 0;
  }

  return meta::hash::multi_hash(
      std::string(static_cast<char const *>(hash_buf), hash_size),
      port);
}



/*****************************************************************************
 * Friend functions
 **/
std::ostream &
operator<<(std::ostream & os, socket_address const & addr)
{
  os << addr.full_str();
  return os;
}


}} // namespace packeteer::net
