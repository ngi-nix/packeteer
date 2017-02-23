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

#include <packeteer/net/detail/cidr.h>

#include <cstdlib>
#include <cstring>
#include <vector>

namespace packeteer {
namespace net {
namespace detail {


ssize_t
parse_extended_cidr(std::string const & _cidr, bool no_mask,
    address_type & address, sa_family_t & proto, uint16_t port /* = 0 */)
{
  // Initialize return value for proto
  proto = AF_UNSPEC;

  // We need to copy the cidr string because we need to manipulate it during
  // this function (a little). Because of the memory management and layout
  // guarantees, we're copying into a vector<char>, though.
  std::vector<char> cidr;
  size_t cidr_size = _cidr.length();
  cidr.reserve(cidr_size + 1);
  ::memcpy(&cidr[0], _cidr.c_str(), cidr_size);
  cidr[cidr_size] = '\0';

  // Locate the delimiter between the IP address and netmask, a '/'.
  char * mask_ptr = ::strstr(&cidr[0], "/");

  // We will not tolerate a mask if no_mask is set. Otherwise, we'll temporarily
  // end the cidr copy at the mask.
  if (nullptr != mask_ptr) {
    if (no_mask) {
      return -1;
    }
    *mask_ptr = '\0';
  }

  // Let's see if we've got a port part. At this point, we need to parse a
  // little bit by hand:
  // - For IPv4 addresses, a colon (address-port-delimiter) is not a valid
  //   character, so finding it is an indicator that a port is specified.
  // - For IPv6 addresses, a colon (address-port-delimiter) is a valid
  //   character. If a port is specified, the address part is enclosed in
  //   square brackets.
  // The best strategy appears to be to check if there is a part enclosed
  // in square brackets. Then try to find a colon behind it (or from the start,
  // if no square brackets are found), and a port behind that.
  char * cidr_start = &cidr[0];
  char * port_ptr = nullptr;
  bool skip_brace = false;
  if ('[' == cidr[0]) {
    port_ptr = ::strstr(&cidr[1], "]:");
    if (nullptr != port_ptr) {
      cidr_start = &cidr[1];
      skip_brace = true;
    }
  }
  else {
    port_ptr = ::strstr(&cidr[0], ":");
  }
  if (nullptr != port_ptr) {
    // Now if there are any non-numeric characters in the port part, we'll
    // know this isn't actually a port, but an IPv6 address (presumably!)
    // without the enclosing braces.
    char * test = port_ptr + 1;
    if (skip_brace) {
      ++test;
    }
    for ( ; *test ; ++test) {
      if (*test < '0' || *test > '9') {
        port_ptr = nullptr;
        break;
      }
    }
  }
  if (nullptr != port_ptr) {
    if (nullptr != mask_ptr) {
      return -1;
    }
    *port_ptr = '\0';
  }

  // Alright, parse port (if necessary).
  uint16_t detected_port = port;
  if (!detected_port && port_ptr) {
    char * parse = port_ptr + 1;
    if (skip_brace) {
      ++parse;
    }
    detected_port = ::atoi(parse);
  }

  // Now try to parse the cidr as an IPv4 or IPv6 address. One of them will
  // succeed (hopefully). We'll use the buffer passed into this function for
  // that.
  if (1 == inet_pton(AF_INET, cidr_start, &(address.sa_in.sin_addr))) {
    // It's IPv4.
    address.sa_in.sin_family = proto = AF_INET;
    address.sa_in.sin_port = htons(detected_port);
  }
  else if (1 == inet_pton(AF_INET6, cidr_start, &(address.sa_in6.sin6_addr))) {
    // It's IPv6.
    address.sa_in6.sin6_family = proto = AF_INET6;
    address.sa_in6.sin6_port = htons(detected_port);
  }
  else {
    // Invalid address
    proto = AF_UNSPEC;
    return -1;
  }

  // If we don't care about a mask, we're done.
  if (no_mask) {
    return 0;
  }

  // If we do care, but don't have one, we're failing.
  if (nullptr == mask_ptr) {
    return -1;
  }

  // Now if we have a netmask, we want to parse it's value.
  ssize_t mask = ::atoi(mask_ptr + 1);
  if (mask <= 0) {
    return -1; // Invalid mask.
  }
  if ((AF_INET == proto && mask > 32)
      || (AF_INET6 == proto && mask > 128))
  {
    return -1; // Invalid mask.
  }
  return mask;
}


}}} // namespace packeteer::net::detail
