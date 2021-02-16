/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2020 Jens Finkhaeuser.
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
#include <packeteer/resolver.h>

#include <unordered_map>

#include <liberate/string/util.h>
#include <liberate/net/address_type.h>
#include <liberate/net/socket_address.h>
#include <liberate/net/resolve.h>

#include "macros.h"

#define FAIL_FAST(expr) { \
    auto err = expr; \
    if (ERR_SUCCESS != err) {\
      throw exception(err); \
    } \
  }

namespace packeteer {

namespace {

inline liberate::net::url
inet_copy_url_with_type(liberate::net::address_type type,
    liberate::net::url const & original)
{
  auto copy = original;
  if (type == liberate::net::AT_INET4) {
    copy.scheme = original.scheme.substr(0, 3) + "4";
  }
  else if (type == liberate::net::AT_INET6) {
    copy.scheme = original.scheme.substr(0, 3) + "6";
  }

  return copy;
}


error_t
inet_resolver(std::shared_ptr<api> api,
    std::set<liberate::net::url> & result,
    liberate::net::url const & query)
{
  // First step: decide which IPs to query for, AT_INET4 or AT_INET6 or
  // AT_UNSPEC.
  auto query_type = liberate::net::AT_UNSPEC;
  if (query.scheme.back() == '4') {
    query_type = liberate::net::AT_INET4;
  }
  else if (query.scheme.back() == '6') {
    query_type = liberate::net::AT_INET6;
  }

  // Now find the query name - that's the authority, but the authority may
  // contain a port.
  // To make matters worse. if handed an IPv6 address, we can't even easily
  // look for the last colon... but fortunately, all of this parsing is
  // already handled in the socket_address code.
  auto sockaddr = liberate::net::socket_address{query.authority};

  // The good news? We can now easily determine if the address is already
  // canonical, and bail out early if that's the case.
  switch (sockaddr.type()) {
    case liberate::net::AT_INET4:
    case liberate::net::AT_INET6:
      {
        auto copy = inet_copy_url_with_type(sockaddr.type(), query);
        result.insert(copy);
      }
      return ERR_SUCCESS;

    default:
      break;
  }

  // If we don't have a canonical IP address in the authority, then we must
  // effectively expect a AT_LOCAL sockaddr - because valid host names are
  // also valid file names.
  if (sockaddr.type() == liberate::net::AT_UNSPEC) {
    ELOG("Malformed URL authority, cannot resolve.");
    return ERR_INVALID_VALUE;
  }

  // Now we know we didn't get an IP address, we can search for the last colon
  // as a port separator.
  std::string hostname = query.authority;
  std::string port_part;

  auto colon = hostname.find_last_of(':');
  if (colon != std::string::npos) {
    port_part = hostname.substr(colon);
    hostname = hostname.substr(0, colon);
  }

  // With this host, we can now use liberate to resolve it to IP addresses.
  std::set<liberate::net::socket_address> addresses;
  try {
    addresses = liberate::net::resolve(api->liberate(), query_type, hostname);
  } catch (std::exception const & ex) {
    EXC_LOG("Error resolving host name", ex);
    return ERR_ADDRESS_NOT_AVAILABLE;
  }

  // Add to results
  for (auto res : addresses) {
    auto copy = inet_copy_url_with_type(res.type(), query);
    if (port_part.empty()) {
      copy.authority = res.cidr_str();
    }
    else {
      if (res.type() == liberate::net::AT_INET4) {
        copy.authority = res.cidr_str() + port_part;
      }
      else {
        copy.authority = "[" + res.cidr_str() + "]" + port_part;
      }
    }
    result.insert(copy);
  }

  return ERR_SUCCESS;
}


} // anonymous namespace

struct resolver::resolver_impl
{
  resolver_impl(std::weak_ptr<api> api)
    : m_api(api)
  {
    init_resolution_funcs();
  }

  std::weak_ptr<api> m_api;

  resolver_impl() = delete;

  /***************************************************************************
   * Resolution mapping
   */
  std::unordered_map<
    std::string, resolver::resolution_function
  > resolution_functions;


  void init_resolution_funcs()
  {
    if (!resolution_functions.empty()) {
      return;
    }

    DLOG("Initializing default resolution functions.");

    // TCP/IP
    FAIL_FAST(register_resolution_function("tcp", inet_resolver));
    FAIL_FAST(register_resolution_function("tcp4", inet_resolver));
    FAIL_FAST(register_resolution_function("tcp6", inet_resolver));

    // UDP/IP
    FAIL_FAST(register_resolution_function("udp", inet_resolver));
    FAIL_FAST(register_resolution_function("udp4", inet_resolver));
    FAIL_FAST(register_resolution_function("udp6", inet_resolver));
  }


  error_t
  register_resolution_function(std::string const & scheme,
        resolution_function && resolution_func)
  {
    if (scheme.empty()) {
      ELOG("Must specify a scheme!");
      return ERR_INVALID_VALUE;
    }

    if (!resolution_func) {
      ELOG("No resolution function provided!");
      return ERR_EMPTY_CALLBACK;
    }

    auto normalized = liberate::string::to_lower(scheme);

    auto func_iter = resolution_functions.find(normalized);
    if (func_iter != resolution_functions.end()) {
      ELOG("Schema already registered!");
      return ERR_INVALID_VALUE;
    }

    // All good, so keep this.
    resolution_functions[normalized] = std::move(resolution_func);
    return ERR_SUCCESS;
  }



  error_t
  resolve(std::set<liberate::net::url> & result,
        liberate::net::url const & query)
  {
    auto normalized = liberate::string::to_lower(query.scheme);

    // Try to find resolution function
    auto func_iter = resolution_functions.find(normalized);
    if (func_iter == resolution_functions.end()) {
      ELOG("Scheme not recognized!");
      return ERR_INVALID_VALUE;
    }

    // Create temporary shared_ptr for API, then call function with that.
    try {
      liberate::net::url query_copy{query};
      query_copy.scheme = normalized;
      return func_iter->second(std::shared_ptr<api>{m_api}, result, query_copy);
    } catch (std::bad_weak_ptr const & ex) {
      ELOG("API is already being destroyed.");
      return ERR_UNEXPECTED;
    }
  }
};




resolver::resolver(std::weak_ptr<api> api)
  : m_impl{std::make_unique<resolver_impl>(api)}
{
}



resolver::~resolver()
{
}



error_t
resolver::register_resolution_function(std::string const & scheme,
      resolver::resolution_function && resolution_func)
{
  return m_impl->register_resolution_function(scheme,
      std::move(resolution_func));
}



error_t
resolver::resolve(std::set<liberate::net::url> & result,
      liberate::net::url const & query)
{
  return m_impl->resolve(result, query);
}




} // namespace packeteer
