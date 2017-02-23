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

#ifndef PACKETEER_NET_DETAIL_CIDR_H
#define PACKETEER_NET_DETAIL_CIDR_H

// *** Config
#include <packeteer/packeteer.h>
#include <packeteer/net/detail/sysincludes.h>
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
 * Returns the bitmask length. The sockaddr_storage is filled in by this
 * function.
 *
 * Returns -1 if the network specification could not be parsed.
 *
 * If the no_mask flag is set, this function expects *no* netmask part to
 * the string, and can be used to parse IPv4 and IPv6 host addresses. On
 * success, it will return 0.
 *
 * However, the function will then return an error (-1) if the mask is
 * present after all.
 *
 * Also returns the protocol type in the proto value.
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
ssize_t parse_extended_cidr(std::string const & cidr, bool no_mask,
    address_type & address, sa_family_t & proto, uint16_t port = 0);

}}} // namespace packeteer::net::detail

#endif // guard
