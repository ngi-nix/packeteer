/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@unwesen.co.uk>
 *
 * Copyright (c) 2014 Unwesen Ltd.
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
#include <packeteer/connector.h>

#if defined(PACKETEER_HAVE_ARPA_INET_H)
#include <arpa/inet.h>
#endif

#if defined(PACKETEER_HAVE_NETINET_IN_H)
#include <netinet/in.h>
#endif

#include <cctype>

#include <algorithm>
#include <map>

#include <packeteer/error.h>
#include <packeteer/net/socket_address.h>

#include <packeteer/detail/connector_anon.h>
#include <packeteer/detail/connector_local.h>


namespace packeteer {

/*****************************************************************************
 * Helper constructs
 **/

namespace {

inline connector::connector_type
match_scheme(std::string const & scheme)
{
  // Initialize mapping, if not yet done
  static std::map<std::string, connector::connector_type> mapping;
  if (mapping.empty()) {
    mapping["tcp4"] = connector::CT_TCP4;
    mapping["tcp6"] = connector::CT_TCP6;
    mapping["tcp"] = connector::CT_TCP;
    mapping["udp4"] = connector::CT_UDP4;
    mapping["udp6"] = connector::CT_UDP6;
    mapping["udp"] = connector::CT_UDP;
    mapping["anon"] = connector::CT_ANON;
    mapping["file"] = connector::CT_FILE;
    mapping["local"] = connector::CT_LOCAL;
    mapping["pipe"] = connector::CT_PIPE;
  }

  // Find scheme type
  auto iter = mapping.find(scheme);
  if (mapping.end() == iter) {
    throw exception(ERR_INVALID_VALUE, "Unsupported connector scheme requested.");
  }

  return iter->second;
}



inline std::pair<connector::connector_type, std::string>
split_address(std::string const & address)
{
  // We'll find the first occurrence of a colon; that *should* delimit the scheme.
  auto pos = address.find_first_of(':');
  if (std::string::npos == pos) {
    throw exception(ERR_FORMAT, "No scheme separator found in connector address.");
  }
  // std::cout << "colon at: " << pos << std::endl;

  // We'll then try to see if the characters immediately following are two
  // slashes.
  if (pos + 3 > address.size()) {
    throw exception(ERR_FORMAT, "Bad scheme separator found in connector address.");
  }
  // std::cout << "pos + 1: " << address[pos + 1] << std::endl;
  // std::cout << "pos + 2: " << address[pos + 2] << std::endl;
  if (!('/' == address[pos + 1] && '/' == address[pos + 2])) {
    throw exception(ERR_FORMAT, "Bad scheme separator found in connector address.");
  }

  // Ok, we seem to have a scheme part. Lower-case it.
  std::string scheme = address.substr(0, pos);
  std::transform(scheme.begin(), scheme.end(), scheme.begin(),
      std::bind2nd(std::ptr_fun(&std::tolower<char>), std::locale("")));
  // std::cout << "got scheme: " << scheme << std::endl;

  // Now check if we know the scheme.
  connector::connector_type ctype = match_scheme(scheme);
  // std::cout << "scheme type: " << ctype << std::endl;

  // That's it, return success!
  auto addrspec = address.substr(pos + 3);
  // std::cout << "address spec: " << addrspec << std::endl;
  return std::make_pair(ctype, addrspec);
}


} // anonymous namespace

/*****************************************************************************
 * Connector implementation
 **/
struct connector::connector_impl
{
  connector::connector_type   m_type;
  detail::connector *         m_conn;

  connector_impl(std::string const & address)
    : m_type(CT_UNSPEC)
    , m_conn(nullptr)
  {
    auto pre_parsed = split_address(address);
    if (pre_parsed.first >= connector::CT_TCP4
        && pre_parsed.first <= connector::CT_UDP)
    {
      if (pre_parsed.second.empty()) {
        throw exception(ERR_FORMAT, "Require address part in address string.");
      }

      // Parse socket address.
      auto addr = net::socket_address(pre_parsed.second);

      // Make sure the parsed address type matches the protocol.
      if (net::socket_address::SAT_INET4 == addr.type()) {
        if (connector::CT_TCP4 != pre_parsed.first
            && connector::CT_TCP != pre_parsed.first
            && connector::CT_UDP4 != pre_parsed.first
            && connector::CT_UDP != pre_parsed.first)
        {
          throw exception(ERR_FORMAT, "IPv4 address provided with IPv6 scheme.");
        }
      }
      else if (net::socket_address::SAT_INET6 == addr.type()) {
        if (connector::CT_TCP6 != pre_parsed.first
            && connector::CT_TCP != pre_parsed.first
            && connector::CT_UDP6 != pre_parsed.first
            && connector::CT_UDP != pre_parsed.first)
        {
          throw exception(ERR_FORMAT, "IPv6 address provided with IPv4 scheme.");
        }
      }
      else {
        throw exception(ERR_FORMAT, "Invalid IPv4 or IPv6 address.");
      }

      // Looks good for TCP/UDP style connectors!
      m_type = pre_parsed.first;
      // TODO
    }

    else if (connector::CT_ANON == pre_parsed.first) {
      // Looks good for anonymous connectors
      m_type = pre_parsed.first;

      std::string type = pre_parsed.second;
      std::transform(type.begin(), type.end(), type.begin(),
          std::bind2nd(std::ptr_fun(&std::tolower<char>), std::locale("")));

      bool block = false;
      if (type.empty() || "nonblocking" == type) {
        block = false;
      }
      else if ("blocking" == type) {
        block = true;
      }
      else {
        throw exception(ERR_FORMAT, "Invalid keyword in address string.");
      }

      m_conn = new detail::connector_anon(block);
    }

    else {
      if (pre_parsed.second.empty()) {
        throw exception(ERR_FORMAT, "Require path in address string.");
      }
      // Looks good for non-TCP/UDP style connectors!
      m_type = pre_parsed.first;

      // Instanciate the connector implementation
      switch (m_type) {
        case CT_LOCAL:
          m_conn = new detail::connector_local(pre_parsed.second);
          break;

        case CT_PIPE:
      // TODO
          break;

        case CT_FILE:
      // TODO
          break;

        default:
          throw exception(ERR_UNEXPECTED, "This should not happen.");
          break;
      }
    }
  }


  ~connector_impl()
  {
    delete m_conn;
  }
};

/*****************************************************************************
 * Implementation
 **/
connector::connector(std::string const & address)
  : m_impl(new connector_impl(address))
{
}



connector::~connector()
{
  delete m_impl;
}



connector::connector_type
connector::type() const
{
  return m_impl->m_type;
}



error_t
connector::listen()
{
  if (!m_impl->m_conn) {
    return ERR_INITIALIZATION;
  }
  return m_impl->m_conn->listen();
}



bool
connector::listening() const
{
  if (!m_impl->m_conn) {
    return false;
  }
  return m_impl->m_conn->listening();
}



error_t
connector::connect()
{
  if (!m_impl->m_conn) {
    return ERR_INITIALIZATION;
  }
  return m_impl->m_conn->connect();
}



bool
connector::connected() const
{
  if (!m_impl->m_conn) {
    return false;
  }
  return m_impl->m_conn->connected();
}



int
connector::get_read_fd() const
{
  if (!m_impl->m_conn) {
    return -1;
  }
  return m_impl->m_conn->get_read_fd();
}



int
connector::get_write_fd() const
{
  if (!m_impl->m_conn) {
    return -1;
  }
  return m_impl->m_conn->get_write_fd();
}



error_t
connector::receive(void * buf, size_t bufsize, size_t & bytes_read,
    ::packeteer::net::socket_address & sender)
{
  if (!m_impl->m_conn) {
    return ERR_INITIALIZATION;
  }
  return m_impl->m_conn->receive(buf, bufsize, bytes_read, sender);
}



error_t
connector::send(void const * buf, size_t bufsize, size_t & bytes_written,
    ::packeteer::net::socket_address const & recipient)
{
  if (!m_impl->m_conn) {
    return ERR_INITIALIZATION;
  }
  return m_impl->m_conn->send(buf, bufsize, bytes_written, recipient);
}



error_t
connector::read(void * buf, size_t bufsize, size_t & bytes_read)
{
  if (!m_impl->m_conn) {
    return ERR_INITIALIZATION;
  }
  return m_impl->m_conn->read(buf, bufsize, bytes_read);
}



error_t
connector::write(void const * buf, size_t bufsize, size_t & bytes_written)
{
  if (!m_impl->m_conn) {
    return ERR_INITIALIZATION;
  }
  return m_impl->m_conn->write(buf, bufsize, bytes_written);
}



error_t
connector::close()
{
  if (!m_impl->m_conn) {
    return ERR_INITIALIZATION;
  }
  return m_impl->m_conn->close();
}


} // namespace packeteer
