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
#ifndef PACKETEER_CONNECTOR_INTERFACE_H
#define PACKETEER_CONNECTOR_INTERFACE_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#include <packeteer/handle.h>
#include <packeteer/error.h>

#include <packeteer/connector/types.h>

#include <packeteer/net/socket_address.h>

namespace packeteer {

/**
 * Base class for connector implementations. See the connector proxy class
 * in the main namespace for details.
 **/
struct PACKETEER_API connector_interface
{
public:
  /***************************************************************************
   * Always to be implemented by child classes
   **/
  /**
   * Either implement, or call from other constructors. Most implementations
   * will accept more constructor parameters.
   */
  connector_interface(connector_options const & options);

  /**
   * Expected to close() the connector.
   */
  virtual ~connector_interface() = 0;

  virtual error_t listen() = 0;
  virtual bool listening() const = 0;

  virtual error_t connect() = 0;
  virtual bool connected() const = 0;

  /**
   * The accept() call *may* return this. The connector proxy class will
   * take care of reference counting such that the connector_interface
   * instance will be deleted only when all connector instances referring to
   * it are gone.
   */
  virtual connector_interface * accept(net::socket_address & addr) const = 0;

  virtual handle get_read_handle() const = 0;
  virtual handle get_write_handle() const = 0;

  virtual error_t close() = 0;

  /***************************************************************************
   * (Abstract) setting accessors
   **/
  /**
   * When setting the blocking mode, be sure to also set it in m_options, or
   * get_blocking_mode() and get_options() may have different opinions.
   */
  virtual error_t get_blocking_mode(bool & state) const = 0;
  virtual error_t set_blocking_mode(bool state) = 0;

  virtual connector_options get_options() const;

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
  connector_options m_options;
};

} // namespace packeteer

#endif // guard
