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
#ifndef PACKETEER_DETAIL_CONNECTOR_H
#define PACKETEER_DETAIL_CONNECTOR_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer/packeteer.h>

#include <packeteer/handle.h>
#include <packeteer/error.h>
#include <packeteer/connector_specs.h>

#include <packeteer/net/socket_address.h>

namespace packeteer {
namespace detail {

/**
 * Base class for connector implementations. See the connector proxy class
 * in the main namespace for details.
 **/
struct connector
{
public:
  /***************************************************************************
   * Always to be implemented by child classes
   **/
  connector(bool blocking = true, connector_behaviour const & behaviour = CB_DEFAULT);

  virtual ~connector() = 0; // Expected to close() the connector

  virtual error_t listen() = 0;
  virtual bool listening() const = 0;

  virtual error_t connect() = 0;
  virtual bool connected() const = 0;

  virtual connector * accept(net::socket_address & addr) const = 0;

  virtual handle get_read_handle() const = 0;
  virtual handle get_write_handle() const = 0;

  virtual error_t close() = 0;

  /***************************************************************************
   * Abstract setting accessors
   **/
  virtual error_t get_blocking_mode(bool & state) const = 0;
  virtual error_t set_blocking_mode(bool state) = 0;

  virtual connector_behaviour get_behaviour() const;

  /***************************************************************************
   * Default (POSIX-oriented) implementations; may be subclassed if necessary.
   **/
  virtual error_t receive(void * buf, size_t bufsize, size_t & bytes_read,
      ::packeteer::net::socket_address & sender);
  virtual error_t send(void const * buf, size_t bufsize, size_t & bytes_written,
      ::packeteer::net::socket_address const & recipient);
  virtual size_t peek() const;

  virtual error_t read(void * buf, size_t bufsize, size_t & bytes_read);
  virtual error_t write(void const * buf, size_t bufsize, size_t & bytes_written);

protected:
  // The connector behaviour is stored here as a reference for derived classes;
  // they can use it to determine how to initialize themselves.
  // It's set during construction, and can afterwards only be read.
  connector_behaviour m_behaviour;

  // The blocking mode is used like the behaviour - however, it will not be
  // modified unless derived classes do so. Derived classes should set and get
  // the blocking mode directly from their underlying handles after
  // construction is complete.
  bool                m_blocking;
};

}} // namespace packeteer::detail

#endif // guard
