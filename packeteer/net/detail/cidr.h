//
//              Project Name: CrowdRoaming
//              file: cidr.hpp
//              copyright: © 2013 DASHT BV, unpublished work.
//                             This computer program includes Confidential, Proprietary Information and is a Trade Secret
//                             of DASHT BV. All use, disclosure, and/or
//                             reproduction is prohibited unless authorized in writing. All Rights Reserved.
//
//

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
 **/
ssize_t parse_cidr(std::string const & cidr, bool no_mask,
    address_type & address, sa_family_t & proto, uint16_t port = 0);

}}} // namespace packeteer::net::detail

#endif // guard
