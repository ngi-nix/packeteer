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

#include <map>
#include <algorithm>

#include <packeteer/util/hash.h>

#include "../macros.h"

namespace packeteer {

namespace {

using schemes_map_t = std::map<connector_type, std::string>;
static const schemes_map_t sc_schemes =
{
  { CT_UNSPEC, "" },
  { CT_TCP4,  "tcp4" },
  { CT_TCP6,  "tcp6" },
  { CT_TCP,   "tcp" },
  { CT_UDP4,  "udp4" },
  { CT_UDP6,  "udp6" },
  { CT_UDP,   "udp" },
  { CT_LOCAL, "local" },
  { CT_PIPE,  "pipe" },
  { CT_ANON,  "anon" },
};



inline connector_type
best_match(connector_type const & ct_type,
    net::address_type const & sa_type)
  OCLINT_SUPPRESS("high cyclomatic complexity")
{
  switch (ct_type) {
    case CT_TCP:
      // TCP must have INET4 or INET6 addresses
      if (net::AT_INET4 == sa_type) {
        return CT_TCP4;
      }
      else if (net::AT_INET6 == sa_type) {
        return CT_TCP6;
      }
      break;

    case CT_TCP4:
      // TCP4 must have INET4 address
      if (net::AT_INET4 == sa_type) {
        return CT_TCP4;
      }
      break;

    case CT_TCP6:
      // TCP6 must have INET6 address
      if (net::AT_INET6 == sa_type) {
        return CT_TCP6;
      }
      break;

    case CT_UDP:
      // UDP must have INET4 or INET6 addresses
      if (net::AT_INET4 == sa_type) {
        return CT_UDP4;
      }
      else if (net::AT_INET6 == sa_type) {
        return CT_UDP6;
      }
      break;

    case CT_UDP4:
      // UDP4 must have INET4 address
      if (net::AT_INET4 == sa_type) {
        return CT_UDP4;
      }
      break;

    case CT_UDP6:
      // UDP6 must have INET6 address
      if (net::AT_INET6 == sa_type) {
        return CT_UDP6;
      }
      break;

    case CT_LOCAL:
    case CT_PIPE:
      // LOCAL and PIPE must have LOCAL address
      if (net::AT_LOCAL == sa_type) {
        return ct_type;
      }
      break;

    case CT_ANON:
    case CT_UNSPEC:
      // Anonymous pipes need unspecified address; so does CT_UNSPEC
      if (net::AT_UNSPEC == sa_type) {
        return ct_type;
      }
      break;

    default:
      PACKETEER_FLOW_CONTROL_GUARD;
  }
  return CT_UNSPEC;
}


inline connector_type
verify_best(connector_type const & ct_type,
    net::address_type const & sa_type)
{
  auto best = best_match(ct_type, sa_type);
  if (CT_UNSPEC == best && ct_type != best) {
    throw exception(ERR_FORMAT, "Connector type does not match address type!");
  }
  return best;
}


inline void
initialize(util::url const & url, net::socket_address & sockaddr,
    connector_type & ctype)
{
  ctype = CT_UNSPEC;
  for (auto entry : sc_schemes) {
    if (entry.second == url.scheme) {
      ctype = entry.first;
      break;
    }
  }

  // Assign address
  switch (ctype) {
    case CT_TCP4:
    case CT_TCP6:
    case CT_TCP:
    case CT_UDP4:
    case CT_UDP6:
    case CT_UDP:
      sockaddr = net::socket_address(url.authority);
      break;

    default:
      sockaddr = net::socket_address(url.path);
      break;
  }

  // Verify
  ctype = verify_best(ctype, sockaddr.type());
}


} // anonymous namespace

peer_address::peer_address(
    connector_type const & type /* = CT_UNSPEC */)
  : m_sockaddr()
  , m_connector_type(type)
{
  m_connector_type = verify_best(type, m_sockaddr.type());
}



peer_address::peer_address(connector_type const & type,
    void const * buf, size_t len)
  : m_sockaddr(buf, len)
  , m_connector_type(type)
{
  m_connector_type = verify_best(type, m_sockaddr.type());
}



peer_address::peer_address(connector_type const & type,
    std::string const & address, uint16_t port /* = 0 */)
  : m_sockaddr(address, port)
  , m_connector_type(type)
{
  m_connector_type = verify_best(type, m_sockaddr.type());
}



peer_address::peer_address(connector_type const & type,
    char const * address, uint16_t port /* = 0 */)
  : m_sockaddr(address, port)
  , m_connector_type(type)
{
  m_connector_type = verify_best(type, m_sockaddr.type());
}



peer_address::peer_address(connector_type const & type,
      ::packeteer::net::socket_address const & address)
  : m_sockaddr(address)
  , m_connector_type(type)
{
  m_connector_type = verify_best(type, m_sockaddr.type());
}



peer_address::peer_address(std::string const & address)
  : m_sockaddr()
  , m_connector_type(CT_UNSPEC)
{
  auto url = ::packeteer::util::url::parse(address);
  initialize(url, m_sockaddr, m_connector_type);
}


peer_address::peer_address(::packeteer::util::url const & url)
  : m_sockaddr()
  , m_connector_type(CT_UNSPEC)
{
  initialize(url, m_sockaddr, m_connector_type);
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
  auto iter = sc_schemes.find(m_connector_type);
  if (iter == sc_schemes.end()) {
    return "";
  }
  return iter->second;
}



std::string
peer_address::str() const
{
  // Scheme
  auto iter = sc_schemes.find(m_connector_type);
  if (iter == sc_schemes.end()) {
    return "";
  }

  return iter->second + "://" + m_sockaddr.full_str();
}



net::socket_address &
peer_address::socket_address()
{
  return m_sockaddr;
}



net::socket_address const &
peer_address::socket_address() const
{
  return m_sockaddr;
}



size_t
peer_address::hash() const
{
  return packeteer::util::multi_hash(
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
