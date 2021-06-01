/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2019-2020 Jens Finkhaeuser.
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
#include <packeteer/registry.h>

#include <unordered_map>

#include <liberate/string/util.h>

#include <packeteer/connector/peer_address.h>

#include "macros.h"
#include "connector/connectors.h"
#include "connector/util.h"


#define FAIL_FAST(expr) { \
    auto err = expr; \
    if (ERR_SUCCESS != err) {\
      throw exception(err); \
    } \
  }


namespace packeteer {

namespace {

connector_interface *
inet_creator(std::shared_ptr<api> api,
    liberate::net::url const & url, connector_type const & ctype,
    connector_options const & options, registry::connector_info const * info)
  OCLINT_SUPPRESS("high cyclomatic complexity")
  OCLINT_SUPPRESS("long method")
{
  if (url.authority.empty()) {
    throw exception(ERR_FORMAT, "Require address part in address string.");
  }

  // Parse socket address.
  auto addr = peer_address{api, url};

  // Make sure the parsed address type matches the protocol.
  if (liberate::net::AT_INET4 == addr.socket_address().type()) {
    if (CT_TCP4 != ctype
        && CT_TCP != ctype
        && CT_UDP4 != ctype
        && CT_UDP != ctype)
    {
      throw exception(ERR_FORMAT, "IPv4 address provided with IPv6 scheme.");
    }
  }
  else if (liberate::net::AT_INET6 == addr.socket_address().type()) {
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

  // Sanitize options
  auto opts = detail::sanitize_options(options, info->default_options,
      info->possible_options);

  // Looks good for TCP/UDP style connectors!
  switch (ctype) {
    case CT_TCP:
    case CT_TCP4:
    case CT_TCP6:
      return new detail::connector_tcp{addr, opts};

    case CT_UDP:
    case CT_UDP4:
    case CT_UDP6:
      return new detail::connector_udp{addr, opts};

    default:
      PACKETEER_FLOW_CONTROL_GUARD;
  }

  return nullptr; // Unreachable; just for clarity.
}


} // anonymous namespace



struct registry::registry_impl
{
  registry_impl(std::weak_ptr<api> api)
    : m_api(api)
  {
    init_params();
    init_schemes();
  }

  std::weak_ptr<api> m_api;


  /***************************************************************************
   * Query parameter registry
   */
  std::unordered_map<std::string, registry::option_mapper> option_mappers;


  void init_params()
  {
    if (!option_mappers.empty()) {
      return;
    }

    DLOG("Initializing default connector URL parameters.");
    FAIL_FAST(add_parameter("behaviour",
        [](std::string const & value, bool) -> connector_options
        {
          // Check for valid behaviour value
          connector_options requested = CO_DEFAULT;
          if ("datagram" == value or "dgram" == value) {
            requested = CO_DATAGRAM;
          } else if ("stream" == value) {
            requested = CO_STREAM;
          }
          return requested;
        }
    ));

    FAIL_FAST(add_parameter("blocking",
        [](std::string const & value, bool) -> connector_options
        {
          // Default to non-blocking options.
          if (value == "1") {
            return CO_BLOCKING;
          }
          return CO_NON_BLOCKING;
        }
    ));

  }



  error_t
  add_parameter(std::string const & parameter, option_mapper && mapper)
  {
    if (parameter.empty()) {
      ELOG("Must specify a URL parameter!");
      return ERR_INVALID_VALUE;
    }
    if (!mapper) {
      ELOG("No mapper function provided!");
      return ERR_EMPTY_CALLBACK;
    }

    auto normalized = liberate::string::to_lower(parameter);

    auto option = option_mappers.find(normalized);
    if (option != option_mappers.end()) {
      ELOG("URL parameter already registered!");
      return ERR_INVALID_VALUE;
    }

    // All good, so keep this.
    option_mappers[normalized] = std::move(mapper);

    return ERR_SUCCESS;
  }



  connector_options
  options_from_query(std::map<std::string, std::string> const & query) const
  {
    connector_options result = CO_DEFAULT;
    for (auto param : option_mappers) {
      DLOG("Checking known option parameter: " << param.first);

      // Get query value for parameter.
      auto query_val = query.find(param.first);
      std::string value;
      bool found = query_val != query.end();
      if (found) {
        value = query_val->second;
      }

      // Invoke mapper
      DLOG("Using mapper to convert value: " << value);
      auto intermediate = param.second(value, found);
      DLOG("Mapper result is: " << intermediate);

      // Success, so set whatever the mapper set.
      result |= intermediate;
    }

    DLOG("Merged options are: " << result);
    return result;
  }


  /***************************************************************************
   * Scheme registry
   */
  std::map<std::string, connector_info> scheme_map;

  void
  init_schemes()
    OCLINT_SUPPRESS("high cyclomatic complexity")
    OCLINT_SUPPRESS("high npath complexity")
  {
    if (!scheme_map.empty()) {
      return;
    }

  // Register TCP and UDP schemes.
  FAIL_FAST(add_scheme("tcp4", connector_info{CT_TCP4,
      CO_STREAM|CO_NON_BLOCKING,
      CO_STREAM|CO_BLOCKING|CO_NON_BLOCKING,
      inet_creator}));
  FAIL_FAST(add_scheme("tcp6", connector_info{CT_TCP6,
      CO_STREAM|CO_NON_BLOCKING,
      CO_STREAM|CO_BLOCKING|CO_NON_BLOCKING,
      inet_creator}));
  FAIL_FAST(add_scheme("tcp", connector_info{CT_TCP,
      CO_STREAM|CO_NON_BLOCKING,
      CO_STREAM|CO_BLOCKING|CO_NON_BLOCKING,
      inet_creator}));

  FAIL_FAST(add_scheme("udp4", connector_info{CT_UDP4,
      CO_DATAGRAM|CO_NON_BLOCKING,
      CO_DATAGRAM|CO_BLOCKING|CO_NON_BLOCKING,
      inet_creator}));
  FAIL_FAST(add_scheme("udp6", connector_info{CT_UDP6,
      CO_DATAGRAM|CO_NON_BLOCKING,
      CO_DATAGRAM|CO_BLOCKING|CO_NON_BLOCKING,
      inet_creator}));
  FAIL_FAST(add_scheme("udp", connector_info{CT_UDP,
      CO_DATAGRAM|CO_NON_BLOCKING,
      CO_DATAGRAM|CO_BLOCKING|CO_NON_BLOCKING,
      inet_creator}));

  // Register anonymous scheme
  FAIL_FAST(add_scheme("anon", connector_info{CT_ANON,
      CO_STREAM|CO_NON_BLOCKING,
      CO_STREAM|CO_BLOCKING|CO_NON_BLOCKING,
      [] (std::shared_ptr<api> api [[maybe_unused]],
          liberate::net::url const & url, connector_type const &,
          connector_options const & options, connector_info const * info)
        -> connector_interface *
      {
        if (!url.path.empty()) {
          throw exception(ERR_FORMAT,
              "Path component makes no sense for anon:// connectors.");
        }

        // Sanitize options
        auto opts = detail::sanitize_options(options, info->default_options,
            info->possible_options);

        return new detail::connector_anon{opts};
      }}));

#if defined(PACKETEER_WIN32)
  // Register pipe scheme
  FAIL_FAST(add_scheme("pipe", connector_info{CT_PIPE,
      CO_STREAM|CO_NON_BLOCKING,
      CO_STREAM|CO_BLOCKING|CO_NON_BLOCKING,
      [] (std::shared_ptr<api> api [[maybe_unused]],
          liberate::net::url const & url, connector_type const &,
          connector_options const & options, connector_info const * info)
        -> connector_interface *
      {
        if (url.path.empty()) {
          throw exception(ERR_FORMAT, "Pipe connectors need a path.");
        }

        // Sanitize options
        auto opts = detail::sanitize_options(options, info->default_options,
            info->possible_options);

        // FIXME
        return new detail::connector_pipe(url.path, opts);
      }}));
#endif

#if defined(PACKETEER_POSIX)
  // Register fifo scheme
  FAIL_FAST(add_scheme("fifo", connector_info{CT_FIFO,
      CO_STREAM|CO_NON_BLOCKING,
      CO_STREAM|CO_BLOCKING|CO_NON_BLOCKING,
      [] (std::shared_ptr<api> api [[maybe_unused]],
          liberate::net::url const & url, connector_type const &,
          connector_options const & options, connector_info const * info)
        -> connector_interface *
      {
        if (url.path.empty()) {
          throw exception(ERR_FORMAT, "FIFO connectors need a path.");
        }

        // Sanitize options
        auto opts = detail::sanitize_options(options, info->default_options,
            info->possible_options);

        return new detail::connector_fifo{peer_address{api, url}, opts};
      }}));
#endif

#if defined(PACKETEER_POSIX) || defined(PACKETEER_HAVE_AFUNIX_H)
  // Register posix local addresses, if possible.
  FAIL_FAST(add_scheme("local", connector_info{CT_LOCAL,
      CO_STREAM|CO_NON_BLOCKING,
      CO_STREAM|CO_DATAGRAM|CO_BLOCKING|CO_NON_BLOCKING,
      [] (std::shared_ptr<api> api [[maybe_unused]],
          liberate::net::url const & url, connector_type const &,
          connector_options const & options, connector_info const * info)
        -> connector_interface *
      {
        // Sanitize options
        auto opts = detail::sanitize_options(options, info->default_options,
            info->possible_options);

        return new detail::connector_local{peer_address{api, url}, opts};
      }}));
#endif
  }



  error_t
  add_scheme(std::string const & scheme, connector_info const & info)
  {
    if (scheme.empty()) {
      ELOG("Must specify a scheme!");
      return ERR_INVALID_VALUE;
    }

    if (CT_UNSPEC == info.type) {
      ELOG("Must specify a type!");
      return ERR_INVALID_VALUE;
    }

    // Multiple schemes may return the same type.

    // Default options may be empty, meaning CO_DEFAULT should be
    // passed. possible_options may be empty, meaning CO_DEFAULT is the only
    // option allowed.

    if (!info.creator) {
      ELOG("Must provide a creator function!");
      return ERR_EMPTY_CALLBACK;
    }

    auto normalized = liberate::string::to_lower(scheme);

    auto scheme_info = scheme_map.find(normalized);
    if (scheme_info != scheme_map.end()) {
      ELOG("Scheme already registered!");
      return ERR_INVALID_VALUE;
    }

    // All good, keep this.
    auto clone = info;
    clone.scheme = normalized;
    scheme_map[normalized] = clone;
    return ERR_SUCCESS;
  }



  connector_info
  info_for_scheme(std::string const & scheme) const
  {
    auto normalized = liberate::string::to_lower(scheme);

    auto scheme_info = scheme_map.find(normalized);
    if (scheme_info == scheme_map.end()) {
      throw exception(ERR_INVALID_VALUE, "Unknown scheme: " + scheme);
    }
    return scheme_info->second;
  }



  connector_info
  info_for_type(connector_type type) const
  {
    for (auto const & entry : scheme_map) {
      if (entry.second.type == type) {
        return entry.second;
      }
    }
    throw exception(ERR_INVALID_VALUE, "Unknown connector type: " + std::to_string(type));
  }
};



registry::registry(std::weak_ptr<api> api)
  : m_impl{std::make_unique<registry_impl>(api)}
{
}



registry::~registry()
{
}



error_t
registry::add_parameter(std::string const & parameter,
    registry::option_mapper && mapper)
{
  return m_impl->add_parameter(parameter, std::move(mapper));
}



connector_options
registry::options_from_query(
      std::map<std::string, std::string> const & query) const
{
  return m_impl->options_from_query(query);
}



error_t
registry::add_scheme(std::string const & scheme, connector_info const & info)
{
  return m_impl->add_scheme(scheme, info);
}



registry::connector_info
registry::info_for_scheme(std::string const & scheme) const
{
  return m_impl->info_for_scheme(scheme);
}



registry::connector_info
registry::info_for_type(connector_type type) const
{
  return m_impl->info_for_type(type);
}

} // namespace packeteer
