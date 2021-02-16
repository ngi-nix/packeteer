/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2020 Jens Finkhaeuser.
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
#ifndef P7R_BENCH_EVENT_BACKENDS_H
#define P7R_BENCH_EVENT_BACKENDS_H

#include <iostream>
#include <vector>
#include <functional>
#include <string>

// For ssize_t
#include <liberate/types.h>

#define _LOG_BASE(opts, os, msg) \
  if (opts.verbose) { \
    os << msg << std::endl; \
  }

#define VERBOSE_LOG(opts, msg) _LOG_BASE(opts, std::cout, msg)
#define VERBOSE_ERR(opts, msg) _LOG_BASE(opts, std::cerr, msg)


enum class backends
{
  PACKETEER = 0,
  LIBEVENT,
  LIBEV,
  LIBUV,
  ASIO,
};

std::ostream & operator<<(std::ostream & os, backends const & b);



struct options
{
  backends    backend = backends::PACKETEER;
  size_t      conns = 100;
  size_t      active = 1;
  size_t      writes = 100;
  uint16_t    port_range_start = 2000;
#if defined _WIN32
  // FIXME: see https://gitlab.com/interpeer/packeteer/-/issues/21
  size_t      runs = 1;
#else
  size_t      runs = 25;
#endif
  bool        verbose = false;
  std::string output_file;
};


struct backend_ops;

using conn_index = size_t;
using read_callback = std::function<void (backend_ops & backend, conn_index index)>;


/**
 * For a new event backend, implement this interface and register it.
 *
 * Note: in order to keep the backend's management of sockets, etc. to the
 *       backend, the main test loop operates on the assumption that each
 *       connection has a unique conn_index, and the conn_index is between
 *       zero and opts.conns.
 */
struct backend_ops
{
  virtual ~backend_ops() {};

  /**
   * Initialize backend. Use this to open sockets, etc.
   */
  virtual void init(options const &) {};

  /**
   * Start and end a single test run. At the start of the test run, a
   * read_callback is to be registered for all connections for the test
   * run to work.
   */
  virtual void start_run(read_callback callback) = 0;
  virtual void end_run() {};

  /**
   * Poll events from the backend once. This should typically register
   * callbacks or fire callbacks due to the writes issued just before.
   */
  virtual void poll_events() = 0;

  /**
   * Backend-specific reading and writing function, operating on the
   * connection indicated by from_idx, and sending to the socket indicated
   * by to_idx.
   *
   * Return true if sendto succeeded, false if it failed. When reading, we
   * care about the amount of bytes received, but not the actual data.
   */
  virtual bool sendto(conn_index from_idx, conn_index to_idx, void const * buf, size_t buflen) = 0;
  virtual ssize_t recv(conn_index from_idx) = 0;
};



void register_backend(backends b,
    std::string const & name,
    std::vector<std::string> const & matches,
    backend_ops * ops);



#endif // P7R_BENCH_EVENT_BACKENDS_H
