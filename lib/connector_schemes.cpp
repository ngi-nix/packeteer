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
#include <build-config.h>

#include "connector_schemes.h"

#include "macros.h"
#include "util/string.h"

#include "detail/connector_tcp.h"
#include "detail/connector_udp.h"
#include "detail/connector_anon.h"
#include "detail/connector_pipe.h"
#if defined(PACKETEER_POSIX)
#  include "detail/connector_local.h"
#endif


namespace PACKETEER_API packeteer {
namespace detail {

namespace {

std::map<std::string, connector_info> scheme_map;


connector_interface *
inet_creator(util::url const & url, connector_type const & ctype,
    connector_options const & options)
{
  if (url.authority.empty()) {
    throw exception(ERR_FORMAT, "Require address part in address string.");
  }

  // Parse socket address.
  auto addr = net::socket_address(url.authority);

  // Make sure the parsed address type matches the protocol.
  if (net::AT_INET4 == addr.type()) {
    if (CT_TCP4 != ctype
        && CT_TCP != ctype
        && CT_UDP4 != ctype
        && CT_UDP != ctype)
    {
      throw exception(ERR_FORMAT, "IPv4 address provided with IPv6 scheme.");
    }
  }
  else if (net::AT_INET6 == addr.type()) {
    if (CT_TCP6 != ctype
        && CT_TCP != ctype
        && CT_UDP6 != ctype
        && CT_UDP != ctype)
    {
      throw exception(ERR_FORMAT, "IPv6 address provided with IPv4 scheme.");
    }
  }
  else {
    throw exception(ERR_FORMAT, "Invalid IPv4 or IPv6 address.");
  }

  // Looks good for TCP/UDP style connectors!
  switch (ctype) {
    case CT_TCP:
    case CT_TCP4:
    case CT_TCP6:
      return new detail::connector_tcp(addr, options);
      break;

    case CT_UDP:
    case CT_UDP4:
    case CT_UDP6:
      return new detail::connector_udp(addr, options);
      break;

    default:
      PACKETEER_FLOW_CONTROL_GUARD;
  }

  return nullptr; // Unreachable; just for clarity.
}


} // anonymous namespace


/**
 * Initialize scheme handlers - functions that create connector_interface
 * implementations.
 */
#define FAIL_FAST(expr) { \
    auto err = expr; \
    if (err) { return err; } \
  }
error_t
init_schemes()
{
  if (!scheme_map.empty()) {
    return ERR_SUCCESS;
  }

  // Register TCP and UDP schemes.
  FAIL_FAST(register_scheme("tcp4", CT_TCP4,
      CO_STREAM|CO_NON_BLOCKING,
      CO_STREAM|CO_BLOCKING|CO_NON_BLOCKING,
      inet_creator));
  FAIL_FAST(register_scheme("tcp6", CT_TCP6,
      CO_STREAM|CO_NON_BLOCKING,
      CO_STREAM|CO_BLOCKING|CO_NON_BLOCKING,
      inet_creator));
  FAIL_FAST(register_scheme("tcp", CT_TCP,
      CO_STREAM|CO_NON_BLOCKING,
      CO_STREAM|CO_BLOCKING|CO_NON_BLOCKING,
      inet_creator));

  FAIL_FAST(register_scheme("udp4", CT_UDP4,
      CO_DATAGRAM|CO_NON_BLOCKING,
      CO_DATAGRAM|CO_BLOCKING|CO_NON_BLOCKING,
      inet_creator));
  FAIL_FAST(register_scheme("udp6", CT_UDP6,
      CO_DATAGRAM|CO_NON_BLOCKING,
      CO_DATAGRAM|CO_BLOCKING|CO_NON_BLOCKING,
      inet_creator));
  FAIL_FAST(register_scheme("udp", CT_UDP,
      CO_DATAGRAM|CO_NON_BLOCKING,
      CO_DATAGRAM|CO_BLOCKING|CO_NON_BLOCKING,
      inet_creator));

  // Register anonymous scheme
  FAIL_FAST(register_scheme("anon", CT_ANON,
      CO_STREAM|CO_NON_BLOCKING,
      CO_STREAM|CO_BLOCKING|CO_NON_BLOCKING,
      [] (util::url const & url, connector_type const &, connector_options const & options) -> connector_interface *
      {
        if (!url.path.empty()) {
          throw exception(ERR_FORMAT, "Path component makes no sense for anon:// connectors.");
        }
        return new detail::connector_anon(options);
      }));

  // Register pipe scheme
  FAIL_FAST(register_scheme("pipe", CT_PIPE,
      CO_STREAM|CO_NON_BLOCKING,
      CO_STREAM|CO_BLOCKING|CO_NON_BLOCKING,
      [] (util::url const & url, connector_type const &, connector_options const & options) -> connector_interface *
      {
        return new detail::connector_pipe(url.path, options);
      }));

#if defined(PACKETEER_POSIX)
  // Register posix local addresses, if possible.
  FAIL_FAST(register_scheme("local", CT_LOCAL,
      CO_STREAM|CO_NON_BLOCKING,
      CO_STREAM|CO_DATAGRAM|CO_BLOCKING|CO_NON_BLOCKING,
      [] (util::url const & url, connector_type const &, connector_options const & options) -> connector_interface *
      {
        return new detail::connector_local(url.path, options);
      }));
#endif
  return ERR_SUCCESS;
}


error_t
register_scheme(std::string const & scheme,
    connector_type const & type,
    connector_options const & default_options,
    connector_options const & possible_options,
    connector::scheme_instantiation_function && creator)
{
  if (scheme.empty()) {
    LOG("Must specify a scheme!");
    return ERR_INVALID_VALUE;
  }

  if (CT_UNSPEC == type) {
    LOG("Must specify a type!");
    return ERR_INVALID_VALUE;
  }

  // Multiple schemes may return the same type.

  // Default options may be empty, meaning CO_DEFAULT should be
  // passed. possible_options may be empty, meaning CO_DEFAULT is the only
  // option allowed.

  if (!creator) {
    LOG("Must provide a creator function!");
    return ERR_EMPTY_CALLBACK;
  }


  auto normalized = util::to_lower(scheme);

  auto scheme_info = scheme_map.find(normalized);
  if (scheme_info != scheme_map.end()) {
    LOG("Scheme already registered!");
    return ERR_INVALID_VALUE;
  }

  // All good, keep this.
  scheme_map[normalized] = connector_info{type, default_options,
    possible_options, std::move(creator)};

  return ERR_SUCCESS;
}



connector_info
info_for_scheme(std::string const & scheme)
{
  auto normalized = util::to_lower(scheme);

  auto scheme_info = scheme_map.find(normalized);
  if (scheme_info == scheme_map.end()) {
    throw exception(ERR_INVALID_VALUE, "Unknown scheme: " + scheme);
  }
  return scheme_info->second;
}



}} // namespace packeteer::detail
