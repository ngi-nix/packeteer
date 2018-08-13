/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2017 Jens Finkhaeuser.
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
#include <packeteer/peer_address.h>

#include <map>

#include <meta/hash.h>

namespace packeteer {

namespace {

typedef std::map<connector::connector_type, std::string> schemes_map_t;
static const schemes_map_t sc_schemes =
{
  { connector::CT_UNSPEC, "" },
  { connector::CT_TCP4,  "tcp4" },
  { connector::CT_TCP6,  "tcp6" },
  { connector::CT_TCP,   "tcp" },
  { connector::CT_UDP4,  "udp4" },
  { connector::CT_UDP6,  "udp6" },
  { connector::CT_UDP,   "udp" },
  { connector::CT_LOCAL, "local" },
  { connector::CT_PIPE,  "pipe" },
  { connector::CT_ANON,  "anon" },
};


inline std::pair<connector::connector_type, std::string>
split_address(std::string const & address)
{
  // We'll find the first occurrence of a colon; that *should* delimit the scheme.
  auto pos = address.find_first_of(':');
  if (std::string::npos == pos) {
    throw exception(ERR_FORMAT, "No scheme separator found in connector address.");
  }

  // We'll then try to see if the characters immediately following are two
  // slashes.
  if (pos + 3 > address.size()) {
    throw exception(ERR_FORMAT, "Bad scheme separator found in connector address.");
  }
  if (!('/' == address[pos + 1] && '/' == address[pos + 2])) {
    throw exception(ERR_FORMAT, "Bad scheme separator found in connector address.");
  }

  // Ok, we seem to have a scheme part. Lower-case it.
  std::string scheme = address.substr(0, pos);
  std::transform(scheme.begin(), scheme.end(), scheme.begin(),
      std::bind2nd(std::ptr_fun(&std::tolower<char>), std::locale("")));

  // Now check if we know the scheme.
  auto iter = std::find_if(sc_schemes.begin(), sc_schemes.end(),
      [&scheme] (schemes_map_t::value_type const & v) {
        return v.second == scheme;
      });

  // That's it, return success!
  auto addrspec = address.substr(pos + 3);
  return std::make_pair(iter->first, addrspec);
}


inline connector::connector_type
best_match(connector::connector_type const & ct_type,
    net::socket_address::socket_address_type const & sa_type)
{
  switch (ct_type) {
    case connector::CT_TCP:
      // TCP must have INET4 or INET6 addresses
      if (net::socket_address::SAT_INET4 == sa_type) {
        return connector::CT_TCP4;
      }
      else if (net::socket_address::SAT_INET6 == sa_type) {
        return connector::CT_TCP6;
      }
      break;

    case connector::CT_TCP4:
      // TCP4 must have INET4 address
      if (net::socket_address::SAT_INET4 == sa_type) {
        return connector::CT_TCP4;
      }
      break;

    case connector::CT_TCP6:
      // TCP6 must have INET6 address
      if (net::socket_address::SAT_INET6 == sa_type) {
        return connector::CT_TCP6;
      }
      break;

    case connector::CT_UDP:
      // UDP must have INET4 or INET6 addresses
      if (net::socket_address::SAT_INET4 == sa_type) {
        return connector::CT_UDP4;
      }
      else if (net::socket_address::SAT_INET6 == sa_type) {
        return connector::CT_UDP6;
      }
      break;

    case connector::CT_UDP4:
      // UDP4 must have INET4 address
      if (net::socket_address::SAT_INET4 == sa_type) {
        return connector::CT_UDP4;
      }
      break;

    case connector::CT_UDP6:
      // UDP6 must have INET6 address
      if (net::socket_address::SAT_INET6 == sa_type) {
        return connector::CT_UDP6;
      }
      break;

    case connector::CT_LOCAL:
    case connector::CT_PIPE:
      // LOCAL and PIPE must have LOCAL address
      if (net::socket_address::SAT_LOCAL == sa_type) {
        return ct_type;
      }
      break;

    case connector::CT_ANON:
    case connector::CT_UNSPEC:
      // Anonymous pipes need unspecified address; so does CT_UNSPEC
      if (net::socket_address::SAT_UNSPEC == sa_type) {
        return ct_type;
      }
      break;
  }
  return connector::CT_UNSPEC;
}


inline connector::connector_type
verify_best(connector::connector_type const & ct_type,
    net::socket_address::socket_address_type const & sa_type)
{
  auto best = best_match(ct_type, sa_type);
  if (connector::CT_UNSPEC == best && ct_type != best) {
    throw exception(ERR_FORMAT, "Connector type does not match address type!");
  }
  return best;
}


} // anonymous namespace

peer_address::peer_address(
    connector::connector_type const & type /* = connector::CT_UNSPEC */)
  : net::socket_address()
  , m_connector_type(type)
{
  m_connector_type = verify_best(type, net::socket_address::type());
}



peer_address::peer_address(connector::connector_type const & type,
    void const * buf, socklen_t len)
  : net::socket_address(buf, len)
  , m_connector_type(type)
{
  m_connector_type = verify_best(type, net::socket_address::type());
}



peer_address::peer_address(connector::connector_type const & type,
    std::string const & address, uint16_t port /* = 0 */)
  : net::socket_address(address, port)
  , m_connector_type(type)
{
  m_connector_type = verify_best(type, net::socket_address::type());
}



peer_address::peer_address(connector::connector_type const & type,
    char const * address, uint16_t port /* = 0 */)
  : net::socket_address(address, port)
  , m_connector_type(type)
{
  m_connector_type = verify_best(type, net::socket_address::type());
}



peer_address::peer_address(connector::connector_type const & type,
      ::packeteer::net::socket_address const & address)
  : net::socket_address(address)
  , m_connector_type(type)
{
  m_connector_type = verify_best(type, net::socket_address::type());
}



peer_address::peer_address(std::string const & address)
  : net::socket_address()
  , m_connector_type(connector::CT_UNSPEC)
{
  // Split address and determine type
  auto pre_parsed = split_address(address);

  // Assign address
  net::socket_address::operator=(net::socket_address(pre_parsed.second));

  // Verify
  m_connector_type = verify_best(pre_parsed.first, net::socket_address::type());
}



connector::connector_type
peer_address::connector_type() const
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

  return iter->second + "://" + this->net::socket_address::full_str();
}



size_t
peer_address::hash() const
{
  return meta::hash::multi_hash(static_cast<int>(m_connector_type),
      *static_cast<net::socket_address const *>(this));
}



void
peer_address::swap(peer_address & other)
{
  net::socket_address::swap(other);

  connector::connector_type tmp = other.m_connector_type;
  other.m_connector_type = m_connector_type;
  m_connector_type = tmp;
}



bool
peer_address::is_equal_to(peer_address const & other) const
{
  if (typeid(this) != typeid(other)) {
    return false;
  }

  if (!net::socket_address::is_equal_to(other)) {
    return false;
  }

  return m_connector_type == other.m_connector_type;
}



bool
peer_address::is_less_than(peer_address const & other) const
{
  if (net::socket_address::is_less_than(other)) {
    return true;
  }

  return m_connector_type < other.m_connector_type;
}



} // namespace packeteer
