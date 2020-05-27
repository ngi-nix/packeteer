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
#include <build-config.h>

#include <packeteer/net/socket_address.h>

#include <cstring>
#include <sstream>

#include <packeteer/error.h>
#include <packeteer/util/hash.h>
#include <packeteer/util/path.h>

#include "cidr.h"


namespace packeteer::net {
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
parse_address(detail::address_data & data, T const & source, uint16_t port)
{
  // Need to zero data.
  ::memset(&data.sa_storage, 0, sizeof(data));

  // Try parsing as a CIDR address.
  detail::parse_result_t result(data);
  error_t err = detail::parse_extended_cidr(source, true, result, port);
  if (ERR_ABORTED != err) {
    return;
  }

#if defined(PACKETEER_HAVE_SOCKADDR_UN)
  // If parsing got aborted, we either have AF_UNIX or AF_UNSPEC as the
  // actual socket type.
#if defined(PACKETEER_WIN32)
  std::string converted = util::to_win32_path(source);
  auto unwrapped = converted.c_str();
#else // PACKETEER_WIN32
  auto unwrapped = unwrap(source);
#endif // PACKETEER_WIN32
  ::memset(&data.sa_storage, 0, sizeof(data));
  data.sa_un.sun_family = ::strlen(unwrapped) > 0 ? AF_UNIX : AF_UNSPEC;
  ::snprintf(data.sa_un.sun_path, UNIX_PATH_MAX, "%s", unwrapped);
#endif
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



socket_address::socket_address(void const * buf, size_t len)
{
  if (static_cast<size_t>(len) > sizeof(data)) {
    throw exception(ERR_INVALID_VALUE, "socket_address: input buffer too "
        "large.");
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
  detail::address_data dummy_addr;
  detail::parse_result_t result(dummy_addr);
  error_t err = detail::parse_extended_cidr(address, true, result);
  return ERR_SUCCESS == err;
}



bool
socket_address::verify_netmask(size_t const & netmask) const
{
  if (AF_INET == data.sa_storage.ss_family) {
    return netmask <= 32;
  }
  else if (AF_INET6 == data.sa_storage.ss_family) {
    return netmask <= 128;
  }
  return false;
}



std::string
socket_address::cidr_str() const
{
  // Interpret data as sockaddr_storage, sockaddr_in and sockaddr_in6. Of the
  // latter two, only one is safe to use!
  char buf[INET6_ADDRSTRLEN] = { '\0' };
  if (AF_INET == data.sa_storage.ss_family) {
#if defined(PACKETEER_WIN32)
    PVOID addr = (PVOID) (&(data.sa_in.sin_addr));
#else
    void const * addr = &(data.sa_in.sin_addr);
#endif
    inet_ntop(data.sa_storage.ss_family, addr, buf, sizeof(buf));
  }
  else if (AF_INET6 == data.sa_storage.ss_family) {
#if defined(PACKETEER_WIN32)
    PVOID addr = (PVOID) (&(data.sa_in6.sin6_addr));
#else
    void const * addr = &(data.sa_in6.sin6_addr);
#endif
    inet_ntop(data.sa_storage.ss_family, addr, buf, sizeof(buf));
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
  std::stringstream sstream;

  switch (data.sa_storage.ss_family) {
    case AF_INET:
      sstream << cidr_str() << ":" << port();
      break;

    case AF_INET6:
      sstream << "[" << cidr_str() << "]:" << port();
      break;

#if defined(PACKETEER_HAVE_SOCKADDR_UN)
    case AF_UNIX:
#if defined(PACKETEER_WIN32)
      sstream << util::to_posix_path(data.sa_un.sun_path);
#else // PACKETEER_WIN32
      sstream << data.sa_un.sun_path;
#endif // PACKETEER_WIN32
      break;
#endif

    default:
      break;
  }

  return sstream.str();
}



size_t
socket_address::bufsize() const
{
  switch (data.sa_storage.ss_family) {
    case AF_INET:
      return sizeof(sockaddr_in);

    case AF_INET6:
      return sizeof(sockaddr_in6);

#if defined(PACKETEER_HAVE_SOCKADDR_UN)
    case AF_UNIX:
      {
        return offsetof(sockaddr_un, sun_path)
          + ::strlen(data.sa_un.sun_path) + 1;
      }
#endif

    default:
      break;
  }
  return 0;
}



size_t
socket_address::bufsize_available() const
{
  return sizeof(data);
}



void const *
socket_address::buffer() const
{
  return &(data.sa_storage);
}



void *
socket_address::buffer()
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
socket_address::is_equal_to(socket_address const & other) const
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

#if defined(PACKETEER_HAVE_SOCKADDR_UN)
  // UNIX
  else {
    compare_size = UNIX_PATH_MAX;
    compare_buf = data.sa_un.sun_path;
    compare_buf_other = other.data.sa_un.sun_path;
  }
#endif

  return 0 == ::memcmp(compare_buf, compare_buf_other, compare_size);
}



bool
socket_address::is_less_than(socket_address const & other) const
{
  // First compare families. It's a bit hard to decide what to return if the
  // families mismatch, but 'false' seems sensible.
  if (data.sa_storage.ss_family != other.data.sa_storage.ss_family) {
    return false;
  }


  // Now compare depending on family. IPv4 is simple.
  if (AF_INET == data.sa_storage.ss_family) {
    if (ntohl(data.sa_in.sin_addr.s_addr)
        < ntohl(other.data.sa_in.sin_addr.s_addr)) {
      return true;
    }
    return ntohs(data.sa_in.sin_port) < ntohs(other.data.sa_in.sin_port);
  }

  // IPv6 is harder, but not too hard. We need to compare byte by byte.
  else if (AF_INET6 == data.sa_storage.ss_family) {
    for (ssize_t i = 0 ; i < 16 ; ++i) {
      if (data.sa_in6.sin6_addr.s6_addr[i]
          < other.data.sa_in6.sin6_addr.s6_addr[i]) {
        return true;
      }
    }

    return ntohs(data.sa_in6.sin6_port) < ntohs(other.data.sa_in6.sin6_port);
  }

#if defined(PACKETEER_HAVE_SOCKADDR_UN)
  // Unix paths are simple again.
  return 0 > ::memcmp(data.sa_un.sun_path, other.data.sa_un.sun_path,
      UNIX_PATH_MAX);
#else
  // We don't know what to do, really.
  return false;
#endif
}



void
socket_address::operator++()
{
  // As always, IPv4 is pretty simple.
  if (AF_INET == data.sa_storage.ss_family) {
    uint32_t ip = ntohl(data.sa_in.sin_addr.s_addr);
    data.sa_in.sin_addr.s_addr = htonl(ip + 1);
    return;
  }
  else if (AF_INET6 == data.sa_storage.ss_family) {
    // IPv6 is still simple enough, we just have to handle overflow from one
    // byte into the next.
    bool done = false;
    for (ssize_t i = 15 ; i >= 0 && !done ; --i) {
      if (data.sa_in6.sin6_addr.s6_addr[i] < 0xff) {
        ++data.sa_in6.sin6_addr.s6_addr[i];
        done = true;
      }
    }
    return;
  }

  throw exception(ERR_UNSUPPORTED_ACTION, "Don't know how to increment paths.");
}



address_type
socket_address::type() const
{
  switch (data.sa_storage.ss_family) {
    case AF_INET:
      return AT_INET4;

    case AF_INET6:
      return AT_INET6;

#if defined(PACKETEER_HAVE_SOCKADDR_UN)
    case AF_UNIX:
      return AT_LOCAL;
#endif

    default:
      return AT_UNSPEC;
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

#if defined(PACKETEER_HAVE_SOCKADDR_UN)
  // UNIX
  else {
    hash_size = ::strlen(data.sa_un.sun_path);
    hash_buf = data.sa_un.sun_path;
    port = 0;
  }
#endif

  return packeteer::util::multi_hash(
      std::string(static_cast<char const *>(hash_buf), hash_size),
      port);
}



void
socket_address::swap(socket_address & other)
{
  void * buf1 = &(data.sa_storage);
  void * buf2 = &(other.data.sa_storage);

  // Use int wise XOR, because int should be most optimized.
  size_t offset = 0;
  size_t max_offset = sizeof(data.sa_storage) / sizeof(int);
  for ( ; offset < max_offset ; ++offset) {
    static_cast<int *>(buf1)[offset] ^= static_cast<int *>(buf2)[offset];
    static_cast<int *>(buf2)[offset] ^= static_cast<int *>(buf1)[offset];
    static_cast<int *>(buf1)[offset] ^= static_cast<int *>(buf2)[offset];
  }

  // Now char-wise until the end of the buffer
  max_offset = sizeof(data.sa_storage) - max_offset;
  offset *= sizeof(int);
  for ( ; offset < max_offset ; ++offset) {
    static_cast<char *>(buf1)[offset] ^= static_cast<char *>(buf2)[offset];
    static_cast<char *>(buf2)[offset] ^= static_cast<char *>(buf1)[offset];
    static_cast<char *>(buf1)[offset] ^= static_cast<char *>(buf2)[offset];
  }
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


} // namespace packeteer::net
