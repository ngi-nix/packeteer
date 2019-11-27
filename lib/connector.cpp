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

#include <packeteer/util/hash.h>

#include <packeteer/error.h>
#include <packeteer/net/address_type.h>
#include <packeteer/net/socket_address.h>

#include "macros.h"
#include "connector_url_params.h"
#include "util/string.h"

#include "detail/connector_anon.h"
#include "detail/connector_pipe.h"
#include "detail/connector_tcp.h"
#include "detail/connector_udp.h"
#if defined(PACKETEER_POSIX)
#  include "detail/connector_local.h"
#endif

namespace packeteer {

/*****************************************************************************
 * Helper constructs
 **/

namespace {

typedef std::pair<
          connector_type,
          std::pair<
            connector_options,   // Default options
            connector_options    // All possible options
          >
        > conn_spec_t;

inline conn_spec_t
match_scheme(std::string const & scheme)
{
  // Initialize mapping, if not yet done
  static std::map<std::string, conn_spec_t> mapping;
  if (mapping.empty()) {
    mapping["tcp4"]  = std::make_pair(CT_TCP4, std::make_pair(CO_STREAM, CO_STREAM|CO_BLOCKING|CO_NON_BLOCKING));
    mapping["tcp6"]  = std::make_pair(CT_TCP6, std::make_pair(CO_STREAM, CO_STREAM|CO_BLOCKING|CO_NON_BLOCKING));
    mapping["tcp"]   = std::make_pair(CT_TCP, std::make_pair(CO_STREAM, CO_STREAM|CO_BLOCKING|CO_NON_BLOCKING));
    mapping["udp4"]  = std::make_pair(CT_UDP4, std::make_pair(CO_DATAGRAM, CO_DATAGRAM|CO_BLOCKING|CO_NON_BLOCKING));
    mapping["udp6"]  = std::make_pair(CT_UDP6, std::make_pair(CO_DATAGRAM, CO_DATAGRAM|CO_BLOCKING|CO_NON_BLOCKING));
    mapping["udp"]   = std::make_pair(CT_UDP, std::make_pair(CO_DATAGRAM, CO_DATAGRAM|CO_BLOCKING|CO_NON_BLOCKING));
    mapping["anon"]  = std::make_pair(CT_ANON, std::make_pair(CO_STREAM, CO_STREAM|CO_BLOCKING|CO_NON_BLOCKING));
    mapping["pipe"]  = std::make_pair(CT_PIPE, std::make_pair(CO_STREAM, CO_STREAM|CO_BLOCKING|CO_NON_BLOCKING));
#if defined(PACKETEER_POSIX)
    mapping["local"] = std::make_pair(CT_LOCAL, std::make_pair(CO_STREAM, CO_STREAM|CO_DATAGRAM|CO_BLOCKING|CO_NON_BLOCKING));
#endif
  }

  // Find scheme type
  auto iter = mapping.find(scheme);
  if (mapping.end() == iter) {
    throw exception(ERR_INVALID_VALUE, "Unsupported connector scheme requested.");
  }

  return iter->second;
}


inline void
connector_schemes_init()
{
  LOG("Initializing default connector schemes.");
  // TODO
}

inline void
connector_init()
{
  error_t err = detail::init_url_params();
  if (ERR_SUCCESS != err) {
    throw exception(err);
  }

  // FIXME
  connector_schemes_init();
}

} // anonymous namespace

/*****************************************************************************
 * Connector implementation
 **/
struct connector::connector_impl
{
  connector_type            m_type;
  connector_options         m_default_options;
  connector_options         m_possible_options;
  util::url                 m_url;
  peer_address              m_address;
  connector_interface *     m_iconn;
  volatile size_t           m_refcount;

  connector_impl(util::url const & connect_url, connector_interface * iconn)
    : m_type(CT_UNSPEC)
    , m_default_options(CO_DEFAULT)
    , m_possible_options(CO_DEFAULT)
    , m_url(connect_url)
    , m_address(m_url)
    , m_iconn(iconn)
    , m_refcount(1)
  {
    connector_init();

    // We don't really need to validate the address here any further, because
    // it's not set by an outside caller - it comes directly from this file, or
    // any of the connector_interface implementations.
    auto spec = match_scheme(m_url.scheme);
    m_type = spec.first;
    m_default_options = spec.second.first;
    m_possible_options = spec.second.second;
  }



  connector_impl(util::url const & connect_url)
    : m_type(CT_UNSPEC)
    , m_default_options(CO_DEFAULT)
    , m_possible_options(CO_DEFAULT)
    , m_url(connect_url)
    , m_address(m_url)
    , m_iconn(nullptr)
    , m_refcount(1)
  {
    connector_init();

    // Find the scheme spec
    auto spec = match_scheme(m_url.scheme);
    auto ctype = spec.first;
    m_default_options = spec.second.first;
    m_possible_options = spec.second.second;

    // Set options - this may be overridden.
    connector_options options = m_default_options;

    // Check if there is a "options" parameter in the url.
    auto requested = detail::options_from_url_params(m_url.query);
    if (requested != CO_DEFAULT) {
      // Ensure the requested value is valid.
      if (!(m_possible_options & requested)) {
        throw exception(ERR_FORMAT, "The requested options are not supported by the connector type!");
      }
      options = requested;
    }

    // Sanity check options - the flags are mutually exclusive.
    if (options & CO_STREAM and options & CO_DATAGRAM) {
      throw exception(ERR_INVALID_OPTION, "Cannot choose both stream and datagram behaviour!");
    }
    if (options & CO_BLOCKING and options & CO_NON_BLOCKING) {
      throw exception(ERR_INVALID_OPTION, "Cannot choose both blocking and non-blocking mode!");
    }
    LOG("Got connector options: " << options << " for type " << ctype);

    // Check connector type
    if (ctype >= CT_TCP4 && ctype <= CT_UDP)
    {
      if (m_url.authority.empty()) {
        throw exception(ERR_FORMAT, "Require address part in address string.");
      }

      // Parse socket address.
      auto addr = net::socket_address(m_url.authority);

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
      m_type = ctype;

      switch (m_type) {
        case CT_TCP:
        case CT_TCP4:
        case CT_TCP6:
          m_iconn = new detail::connector_tcp(addr, options);
          break;

        case CT_UDP:
        case CT_UDP4:
        case CT_UDP6:
          m_iconn = new detail::connector_udp(addr, options);
          break;

        default:
          PACKETEER_FLOW_CONTROL_GUARD;
      }
    }

    else if (CT_ANON == ctype) {
      // Looks good for anonymous connectors
      if (!m_url.path.empty()) {
        throw exception(ERR_FORMAT, "Path component makes no sense for anon:// connectors.");
      }
      m_type = ctype;
      m_iconn = new detail::connector_anon(options);
    }

    else {
      if (m_url.path.empty()) {
        throw exception(ERR_FORMAT, "Require path in address string.");
      }
      // Pass on blocking mode.

      // Looks good for non-TCP/UDP style connectors!
      m_type = ctype;

      // Instanciate the connector implementation
      switch (m_type) {
#if defined(PACKETEER_POSIX)
        case CT_LOCAL:
          m_iconn = new detail::connector_local(m_url.path, options);
          break;
#endif

        case CT_PIPE:
          m_iconn = new detail::connector_pipe(m_url.path, options);
          break;

        default:
          throw exception(ERR_UNEXPECTED, "This should not happen.");
          break;
      }
    }
  }



  ~connector_impl()
  {
    delete m_iconn;
  }


  connector_interface & operator*()
  {
    return *m_iconn;
  }


  connector_interface * operator->()
  {
    return m_iconn;
  }


  operator bool() const
  {
    return m_iconn != nullptr;
  }


  size_t hash() const
  {
    size_t value = packeteer::util::multi_hash(
        static_cast<int>(m_type),
        m_url);

    if (m_iconn) {
      packeteer::util::hash_combine(value,
          packeteer::util::multi_hash(
            m_iconn->get_read_handle(),
            m_iconn->get_write_handle()));
    }
    return value;
  }
};

/*****************************************************************************
 * Implementation
 **/
connector::connector(std::string const & connect_url)
  : m_impl(new connector_impl(util::url::parse(connect_url)))
{
}



connector::connector(util::url const & connect_url)
  : m_impl(new connector_impl(connect_url))
{
}



connector::connector(connector const & other)
  : m_impl(other.m_impl)
{
  if (m_impl) {
    ++(m_impl->m_refcount);
  }
}



connector::connector()
  : m_impl(nullptr)
{
}



connector::~connector()
{
  if (m_impl && --(m_impl->m_refcount) <= 0) {
    delete m_impl;
  }
  m_impl = nullptr;
}



connector_type
connector::type() const
{
  if (!m_impl) {
    return CT_UNSPEC;
  }
  return m_impl->m_type;
}



util::url
connector::connect_url() const
{
  if (!m_impl) {
    throw exception(ERR_INITIALIZATION, "Connector not initialized.");
  }
  return m_impl->m_url;
}



net::socket_address
connector::socket_address() const
{
  if (!m_impl) {
    throw exception(ERR_INITIALIZATION, "Connector not initialized.");
  }
  return m_impl->m_address.socket_address();
}



peer_address
connector::peer_addr() const
{
  if (!m_impl) {
    throw exception(ERR_INITIALIZATION, "Connector not initialized.");
  }
  return m_impl->m_address;
}



error_t
connector::listen()
{
  if (!m_impl || !*m_impl) {
    return ERR_INITIALIZATION;
  }
  return (*m_impl)->listen();
}



bool
connector::listening() const
{
  if (!m_impl || !*m_impl) {
    return false;
  }
  return (*m_impl)->listening();
}



error_t
connector::connect()
{
  if (!m_impl || !*m_impl) {
    return ERR_INITIALIZATION;
  }
  return (*m_impl)->connect();
}



bool
connector::connected() const
{
  if (!m_impl || !*m_impl) {
    return false;
  }
  return (*m_impl)->connected();
}



connector
connector::accept() const
{
  if (!m_impl || !*m_impl) {
    throw exception(ERR_INITIALIZATION, "Can't accept() an uninitialized connector!");
  }

  if (!listening()) {
    throw exception(ERR_UNSUPPORTED_ACTION, "Can't accept() on a non-server connector!");
  }

  net::socket_address peer;
  connector_interface * iconn = (*m_impl)->accept(peer);

  // 1. If we have a socket address in the result, that'll be the best choice
  //    for the implementation's address. Otherwise pass this object's address
  //    (e.g. for anon connectors).
  // 2. Some connectors return themselves, in which case we want to use our own
  //    m_impl and bump the ref count. However, if we have a different address
  //    (see above), that won't work.
  connector result;
  if (net::AT_UNSPEC == peer.type()) {
    if (iconn == m_impl->m_iconn) {
      // Connectors and address are identical
      result.m_impl = m_impl;
      ++(m_impl->m_refcount);
    }
    else {
      // Address is identical, but connector is not
      result.m_impl = new connector_impl(m_impl->m_url, iconn);
    }
  }
  else {
    // We have a new address, so we always need a new impl object.
    // This would lead to a double delete on the conn object, so we'll
    // have to prevent that.
    if (iconn == m_impl->m_iconn) {
      delete iconn;
      iconn = nullptr;
      throw exception(ERR_UNEXPECTED, "Connector's accept() returned self but with new peer address.");
    }
    LOG("Peer address is: " << peer.full_str() << " - " << iconn);

    result.m_impl = new connector_impl(util::url::parse(peer.full_str()), iconn);
  }

  return result;
}




handle
connector::get_read_handle() const
{
  if (!m_impl || !*m_impl) {
    return handle();
  }
  return (*m_impl)->get_read_handle();
}



handle
connector::get_write_handle() const
{
  if (!m_impl || !*m_impl) {
    return handle();
  }
  return (*m_impl)->get_write_handle();
}



error_t
connector::receive(void * buf, size_t bufsize, size_t & bytes_read,
    net::socket_address & sender)
{
  if (!m_impl || !*m_impl) {
    return ERR_INITIALIZATION;
  }
  return (*m_impl)->receive(buf, bufsize, bytes_read, sender);
}



error_t
connector::send(void const * buf, size_t bufsize, size_t & bytes_written,
    net::socket_address const & recipient)
{
  if (!m_impl || !*m_impl) {
    return ERR_INITIALIZATION;
  }
  return (*m_impl)->send(buf, bufsize, bytes_written, recipient);
}



error_t
connector::receive(void * buf, size_t bufsize, size_t & bytes_read,
    peer_address & sender)
{
  if (!m_impl || !*m_impl) {
    return ERR_INITIALIZATION;
  }

  error_t err = (*m_impl)->receive(buf, bufsize, bytes_read, sender.socket_address());
  sender.conn_type() = m_impl->m_address.conn_type();
  return err;
}



error_t
connector::send(void const * buf, size_t bufsize, size_t & bytes_written,
    peer_address const & recipient)
{
  if (!m_impl || !*m_impl) {
    return ERR_INITIALIZATION;
  }
  return (*m_impl)->send(buf, bufsize, bytes_written, recipient.socket_address());
}



size_t
connector::peek() const
{
  if (!m_impl || !*m_impl) {
    throw exception(ERR_INITIALIZATION);
  }
  return (*m_impl)->peek();
}



error_t
connector::read(void * buf, size_t bufsize, size_t & bytes_read)
{
  if (!m_impl || !*m_impl) {
    return ERR_INITIALIZATION;
  }
  return (*m_impl)->read(buf, bufsize, bytes_read);
}



error_t
connector::write(void const * buf, size_t bufsize, size_t & bytes_written)
{
  if (!m_impl || !*m_impl) {
    return ERR_INITIALIZATION;
  }
  return (*m_impl)->write(buf, bufsize, bytes_written);
}



error_t
connector::close()
{
  if (!m_impl || !*m_impl) {
    return ERR_INITIALIZATION;
  }
  return (*m_impl)->close();
}



error_t
connector::set_blocking_mode(bool state)
{
  if (!m_impl || !*m_impl) {
    return ERR_INITIALIZATION;
  }
  return (*m_impl)->set_blocking_mode(state);
}



error_t
connector::get_blocking_mode(bool & state)
{
  if (!m_impl || !*m_impl) {
    return ERR_INITIALIZATION;
  }
  return (*m_impl)->get_blocking_mode(state);
}



connector_options
connector::get_options() const
{
  if (!m_impl || !*m_impl) {
    throw exception(ERR_INITIALIZATION, "Error retrieving options.");
  }
  return (*m_impl)->get_options();
}



connector::operator bool() const
{
  return type() != CT_UNSPEC;
}



bool
connector::is_equal_to(connector const & other) const
{
  bool ret = type() == other.type();
  if (type() == CT_UNSPEC) {
    return ret;
  }
  return ret && (
      get_read_handle() == other.get_read_handle()
      && get_write_handle() == other.get_write_handle()
      && connect_url() == other.connect_url()
  );
}



bool
connector::is_less_than(connector const & other) const
{
  if (type() == CT_UNSPEC) {
    return true;
  }
  if (type() < other.type()) {
    return true;
  }
  if (get_read_handle() < other.get_read_handle()) {
    return true;
  }
  if (get_write_handle() < other.get_write_handle()) {
    return true;
  }
  return connect_url() < other.connect_url();
}



connector &
connector::operator=(connector const & other)
{
  // First decrease the refcount on our existing implementation
  if (m_impl && --(m_impl->m_refcount) <= 0) {
    delete m_impl;
  }

  // Then acept the other's implementation
  m_impl = other.m_impl;
  ++(m_impl->m_refcount);

  return *this;
}



void
connector::swap(connector & other)
{
  std::swap(m_impl, other.m_impl);
}



size_t
connector::hash() const
{
  if (!m_impl || !*m_impl) {
    return 0;
  }
  return m_impl->hash();
}



/***************************************************************************
 * Registry interface
 **/
error_t
connector::register_option(std::string const & url_parameter,
    connector::option_mapping_function && mapper)
{
  return detail::register_url_param(url_parameter, std::move(mapper));
}



} // namespace packeteer
