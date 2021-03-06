/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2017-2020 Jens Finkhaeuser.
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

#include <packeteer/connector/peer_address.h>
#include <packeteer/registry.h>

#include <unordered_map>
#include <algorithm>

#include <liberate/cpp/hash.h>

#include "../macros.h"

namespace packeteer {

namespace {

inline connector_type
best_match(connector_type const & ct_type,
    liberate::net::address_type const & sa_type)
  OCLINT_SUPPRESS("high cyclomatic complexity")
{
  switch (ct_type) {
    case CT_TCP:
      // TCP must have INET4 or INET6 addresses
      if (liberate::net::AT_INET4 == sa_type) {
        return CT_TCP4;
      }
      else if (liberate::net::AT_INET6 == sa_type) {
        return CT_TCP6;
      }
      break;

    case CT_TCP4:
      // TCP4 must have INET4 address
      if (liberate::net::AT_INET4 == sa_type) {
        return CT_TCP4;
      }
      break;

    case CT_TCP6:
      // TCP6 must have INET6 address
      if (liberate::net::AT_INET6 == sa_type) {
        return CT_TCP6;
      }
      break;

    case CT_UDP:
      // UDP must have INET4 or INET6 addresses
      if (liberate::net::AT_INET4 == sa_type) {
        return CT_UDP4;
      }
      // cppcheck-suppress knownConditionTrueFalse
      else if (liberate::net::AT_INET6 == sa_type) {
        return CT_UDP6;
      }
      break;

    case CT_UDP4:
      // UDP4 must have INET4 address
      if (liberate::net::AT_INET4 == sa_type) {
        return CT_UDP4;
      }
      break;

    case CT_UDP6:
      // UDP6 must have INET6 address
      if (liberate::net::AT_INET6 == sa_type) {
        return CT_UDP6;
      }
      break;

    case CT_PIPE:
      // PIPE must have LOCAL address
      if (liberate::net::AT_LOCAL == sa_type) {
        return ct_type;
      }
      break;

    case CT_LOCAL:
      // LOCAL may have a LOCAL or UNSPEC address.
      if (liberate::net::AT_LOCAL == sa_type || liberate::net::AT_UNSPEC == sa_type) {
        return ct_type;
      }
      break;

    case CT_ANON:
    case CT_UNSPEC:
      // Anonymous pipes need unspecified address; so does CT_UNSPEC
      if (liberate::net::AT_UNSPEC == sa_type) {
        return ct_type;
      }
      break;

    default:
      // If we don't know what the best content type is, we just return the
      // given content type. This is used by extended schemes.
      return ct_type;
  }
  return CT_UNSPEC;
}


inline connector_type
verify_best(connector_type const & ct_type,
    liberate::net::address_type const & sa_type)
{
  auto best = best_match(ct_type, sa_type);
  if (CT_UNSPEC == best && ct_type != best) {
    throw exception(ERR_FORMAT, "Connector type does not match address type!");
  }
  return best;
}


inline void
initialize_from_url(
    liberate::net::url const & url,
    liberate::net::socket_address & sockaddr,
    connector_type & ctype)
{
  // Assign address
  switch (ctype) {
    case CT_TCP4:
    case CT_TCP6:
    case CT_TCP:
    case CT_UDP4:
    case CT_UDP6:
    case CT_UDP:
      sockaddr = liberate::net::socket_address{url.authority};
      break;

    case CT_LOCAL:
      // URLs by definition contain *absolute* paths, which means this URL will
      // contain a leading '/'. We therefore cannot detect abstract path names
      // by a leading '\0'; instead, it is the second character that would have
      // to be a zero byte.
      if (url.path.size() > 1 && url.path[1] == '\0') {
        // Got an abstract name - we need to strip the leading slash.
        auto tmp = std::string{url.path.c_str() + 1, url.path.size() - 1};
        sockaddr = liberate::net::socket_address{tmp};
      }
      else {
        // Got an actual path name
        sockaddr = liberate::net::socket_address{url.path};
      }
      break;

    default:
      sockaddr = liberate::net::socket_address{url.path};
      break;
  }

  // Verify
  ctype = verify_best(ctype, sockaddr.type());
}


} // anonymous namespace

peer_address::peer_address()
  : m_sockaddr()
  , m_connector_type(CT_UNSPEC)
  , m_scheme()
{
}



peer_address::peer_address(std::shared_ptr<api> api, std::string const & address)
  : m_sockaddr()
  , m_connector_type(CT_UNSPEC)
{
  // Try to get the best content type from the scheme, and how the address parses
  auto url = liberate::net::url::parse(address);
  auto info = api->reg().info_for_scheme(url.scheme);
  m_connector_type = info.type;
  initialize_from_url(url, m_sockaddr, m_connector_type);

  // Try again, this time with the determined content type
  info = api->reg().info_for_type(m_connector_type);
  m_scheme = info.scheme;
}



peer_address::peer_address(std::shared_ptr<api> api, liberate::net::url const & url)
  : m_sockaddr()
  , m_connector_type(CT_UNSPEC)
{
  // Try to get the best content type from the scheme, and how the address parses
  auto info = api->reg().info_for_scheme(url.scheme);
  m_connector_type = info.type;
  initialize_from_url(url, m_sockaddr, m_connector_type);

  // Try again, this time with the determined content type
  info = api->reg().info_for_type(m_connector_type);
  m_scheme = info.scheme;
}



connector_type &
peer_address::conn_type()
{
  return m_connector_type;
}



connector_type const &
peer_address::conn_type() const
{
  return m_connector_type;
}



std::string
peer_address::scheme() const
{
  return m_scheme;
}



std::string
peer_address::str() const
{
  auto str = m_sockaddr.full_str();
  if (m_sockaddr.type() == liberate::net::AT_LOCAL) {
    if (str[0] == '\0') {
      std::string tmp{"/%00"};
      str = tmp + (str.c_str() + 1);
    }
  }
  return m_scheme + "://" + str;
}



liberate::net::socket_address &
peer_address::socket_address()
{
  return m_sockaddr;
}



liberate::net::socket_address const &
peer_address::socket_address() const
{
  return m_sockaddr;
}



size_t
peer_address::hash() const
{
  return liberate::cpp::multi_hash(
      static_cast<int>(m_connector_type),
      m_sockaddr.hash());
}



void
peer_address::swap(peer_address & other)
{
  std::swap(m_sockaddr, other.m_sockaddr);
  std::swap(m_connector_type, other.m_connector_type);
}



bool
peer_address::is_equal_to(peer_address const & other) const
{
  if (!m_sockaddr.is_equal_to(other.m_sockaddr)) {
    return false;
  }

  return m_connector_type == other.m_connector_type;
}



bool
peer_address::is_less_than(peer_address const & other) const
{
  if (m_sockaddr.is_less_than(other.m_sockaddr)) {
    return true;
  }

  return m_connector_type < other.m_connector_type;
}


/*****************************************************************************
 * Friend functions
 **/
std::ostream &
operator<<(std::ostream & ostream, peer_address const & addr)
{
  ostream << addr.str();
  return ostream;
}


} // namespace packeteer
