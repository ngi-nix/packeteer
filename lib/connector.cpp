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

#include <packeteer/error.h>

#include <packeteer/util/hash.h>

#include <packeteer/net/address_type.h>
#include <packeteer/net/socket_address.h>

#include "macros.h"
#include "connector_url_params.h"
#include "connector_schemes.h"
#include "util/string.h"

namespace packeteer {

/*****************************************************************************
 * Helper constructs
 **/

namespace {

inline void
connector_init()
{
  error_t err = detail::init_url_params();
  if (ERR_SUCCESS != err) {
    throw exception(err);
  }

  err = detail::init_schemes();
  if (ERR_SUCCESS != err) {
    throw exception(err);
  }
}

} // anonymous namespace

/*****************************************************************************
 * Connector implementation
 **/
struct connector::connector_impl
{
  connector_type                            m_type;
  connector_options                         m_default_options;
  connector_options                         m_possible_options;
  connector::scheme_instantiation_function  m_creator;

  util::url                                 m_url;
  peer_address                              m_address;
  connector_interface *                     m_iconn;

  connector_impl(util::url const & connect_url, connector_interface * iconn)
    : m_type(CT_UNSPEC)
    , m_default_options(CO_DEFAULT)
    , m_possible_options(CO_DEFAULT)
    , m_url(connect_url)
    , m_address(m_url)
    , m_iconn(iconn)
  {
    connector_init();

    // We don't really need to validate the address here any further, because
    // it's not set by an outside caller - it comes directly from this file, or
    // any of the connector_interface implementations.
    auto info = detail::info_for_scheme(m_url.scheme);
    m_type = info.type;
    m_default_options = info.default_options;
    m_possible_options = info.possible_options;
    m_creator = info.creator;
  }



  connector_impl(util::url const & connect_url)
    : m_type(CT_UNSPEC)
    , m_default_options(CO_DEFAULT)
    , m_possible_options(CO_DEFAULT)
    , m_url(connect_url)
    , m_address(m_url)
    , m_iconn(nullptr)
  {
    connector_init();

    // Find the scheme spec
    auto info = detail::info_for_scheme(m_url.scheme);
    auto ctype = info.type;
    m_default_options = info.default_options;
    m_possible_options = info.possible_options;
    m_creator = info.creator;

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

    // Try to create the implementation
    auto iconn = m_creator(m_url, ctype, options);
    if (!iconn) {
      throw exception(ERR_INITIALIZATION, "Could not instantiate connector scheme.");
    }

    m_type = ctype;
    m_iconn = iconn;
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
  : m_impl{std::make_shared<connector_impl>(util::url::parse(connect_url))}
{
}



connector::connector(util::url const & connect_url)
  : m_impl{std::make_shared<connector_impl>(connect_url)}
{
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
    }
    else {
      // Address is identical, but connector is not
      result.m_impl = std::make_shared<connector_impl>(m_impl->m_url, iconn);
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

    result.m_impl = std::make_shared<connector_impl>(util::url::parse(peer.full_str()), iconn);
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


error_t
connector::register_scheme(std::string const & scheme,
      connector_type const & type,
      connector_options const & default_options,
      connector_options const & possible_options,
      connector::scheme_instantiation_function && creator)
{
  return detail::register_scheme(scheme, type, default_options,
      possible_options, std::move(creator));
}




} // namespace packeteer
