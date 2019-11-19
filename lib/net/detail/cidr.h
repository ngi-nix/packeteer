/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2014 Unwesen Ltd.
 * Copyright (c) 2015-2019 Jens Finkhaeuser.
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

#ifndef PACKETEER_NET_DETAIL_CIDR_H
#define PACKETEER_NET_DETAIL_CIDR_H

// *** Config
#include <packeteer.h>
#include <packeteer/error.h>
#include <packeteer/detail/netincludes.h>
#include <packeteer/net/socket_address.h>

// *** C++ includes
#include <string>

namespace packeteer {
namespace net {
namespace detail {

/**
 * Parses a CIDR-notation network specification into a socketaddr_storage
 * sized buffer (setting the port part to 0), and a bitmask length.
 *
 * The results are stored in parse_result_t. It will contain the detected
 * protocol type, parsed address, and mask size (if applicable).
 *
 * The return value is an error_t, with ERR_SUCCESS indicating absolute
 * parse success.
 *
 * ERR_INVALID_VALUE is returned if something about the address specification
 * does not meet the requirements, e.g. a CIDR address with a port *and*
 * netmask specification (with port is valid, with netmask is valid, but not
 * both).
 *
 * ERR_ABORTED is returned if no CIDR format could be detected. It's possible
 * that the address is of non-CIDR type, such as a local path.
 *
 * If the no_mask flag is set, this function expects *no* netmask part to
 * the string, and can be used to parse IPv4 and IPv6 host addresses.
 *
 * The CIDR specification is extended in that we also parse ports, if specified.
 * Note that any argument to the port parameter will override the port
 * specification found in the cidr string.
 *
 * For IPv4 and IPv6, the port is specified after the address part, separated by
 * a colon. For IPv6, the address part additionally needs to be enclosed in
 * square brackets. Note that if a port is specified, a netmask cannot be and
 * vice versa.
 **/

struct parse_result_t
{
  inline parse_result_t(address_type & data)
    : proto(AF_UNSPEC)
    , address(data)
    , mask(-1)
  {
  }

  sa_family_t     proto;    // Protocol detection.
  address_type &  address;  // The address.
  ssize_t         mask;     // For AF_INET*
};

packeteer::error_t
parse_extended_cidr(std::string const & cidr, bool no_mask,
    parse_result_t & result, uint16_t port = 0);

}}} // namespace packeteer::net::detail

#endif // guard
