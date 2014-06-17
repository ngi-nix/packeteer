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

#include <cctype>

#include <algorithm>
#include <map>

#include <packeteer/error.h>
#include <packeteer/net/socket_address.h>


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
    mapping["file"] = connector::CT_FILE;
    mapping["ipc"] = connector::CT_IPC;
    mapping["pipe"] = connector::CT_PIPE;
  }

  // Find scheme type
  auto iter = mapping.find(scheme);
  if (mapping.end() == iter) {
    throw std::runtime_error("FIXME");
  }

  return iter->second;
}



inline std::pair<connector::connector_type, std::string>
split_address(std::string const & address)
{
  // We'll find the first occurrence of a colon; that *should* delimit the scheme.
  auto pos = address.find_first_of(':');
  if (std::string::npos == pos) {
    throw std::runtime_error("FIXME");
  }
  // std::cout << "colon at: " << pos << std::endl;

  // We'll then try to see if the characters immediately following are two
  // slashes. We also need a non-empty address, so we'll check that there's
  // space for 1 character more than the two slashes and the colon itself.
  if (pos + 4 > address.size()) {
    throw std::runtime_error("FIXME");
  }
  // std::cout << "pos + 1: " << address[pos + 1] << std::endl;
  // std::cout << "pos + 2: " << address[pos + 2] << std::endl;
  if (!('/' == address[pos + 1] && '/' == address[pos + 2])) {
    throw std::runtime_error("FIXME");
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
  connector_impl(std::string const & address)
  {
    auto pre_parsed = split_address(address);
    if (pre_parsed.first >= connector::CT_TCP4
        && pre_parsed.first <= connector::CT_UDP)
    {
      // Parse socket address.
      auto addr = net::socket_address(pre_parsed.second);

      // Make sure the parsed address type matches the protocol.
      if (net::socket_address::SAT_INET4 == addr.type()) {
        if (connector::CT_TCP4 != pre_parsed.first
            && connector::CT_TCP != pre_parsed.first
            && connector::CT_UDP4 != pre_parsed.first
            && connector::CT_UDP != pre_parsed.first)
        {
          throw std::runtime_error("FIXME");
        }
      }
      if (net::socket_address::SAT_INET6 == addr.type()) {
        if (connector::CT_TCP6 != pre_parsed.first
            && connector::CT_TCP != pre_parsed.first
            && connector::CT_UDP6 != pre_parsed.first
            && connector::CT_UDP != pre_parsed.first)
        {
          throw std::runtime_error("FIXME");
        }
      }

      // Looks good for TCP/UDP style connectors!
      // TODO
    }
    else {
      // Looks good for non-TCP/UDP style connectors!
      // TODO
    }
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

} // namespace packeteer
