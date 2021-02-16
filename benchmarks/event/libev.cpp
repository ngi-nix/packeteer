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
#include <iostream>
#include <cstring>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <ev.h>

#include <liberate/net/socket_address.h>

#include "backends.h"

struct libev_ops;

struct callback_context
{
  libev_ops *   lops;
  read_callback callback;
};


namespace {
  std::vector<ev_io>             watchers;
  std::vector<callback_context>  contexts;
}


static void
read_cb_bridge(EV_P_ ev_io * w, int revents);

struct libev_ops : public backend_ops
{
  virtual ~libev_ops()
  {
    for (conn_index i = 0 ; i < m_conns.size() ; ++i) {
      close(m_conns[i]);
    }
  }



  virtual void init(options const & opts)
  {
    m_opts = opts;

    watchers.resize(opts.conns);
    m_conns.resize(opts.conns);
    m_addrs.resize(opts.conns);

#ifdef _WIN32
    WSADATA WSAData;
    WSAStartup(0x101, &WSAData);
#endif

    uint16_t port = opts.port_range_start;
    for (conn_index i = 0 ; i < opts.conns ; ++i, ++port) {
      auto h = socket(AF_INET, SOCK_DGRAM, 0);
      if (h == -1) {
        throw std::runtime_error("Could not create socket!");
      }
      // FIXME
//      evutil_make_socket_nonblocking(h);
//      evutil_make_socket_closeonexec(h);

      liberate::net::socket_address addr{"127.0.0.1", port};
      auto ret = bind(h, (sockaddr const *) addr.buffer(), addr.bufsize());
      if (ret < 0) {
        throw std::runtime_error("Unable to bind.");
      }

      // All good with this handle
      m_conns[i] = h;
      m_addrs[i] = addr;
    }
  }


  virtual void start_run(read_callback callback)
  {
    contexts.resize(m_opts.conns);

    for (conn_index i = 0 ; i < m_opts.conns; ++i) {
      contexts[i] = { this, callback };
      ev_io_init(&watchers[i], read_cb_bridge, m_conns[i], EV_READ);
      ev_io_start(m_loop, &watchers[i]);
    }
  }



  virtual bool sendto(conn_index from_idx, conn_index to_idx, void const * buf, size_t buflen)
  {
    VERBOSE_LOG(m_opts, "Sending from " << from_idx << " to " << to_idx);

    auto & recipient = m_addrs[to_idx];
    auto err = ::sendto(m_conns[from_idx], buf, buflen, 0,
        (sockaddr const *) recipient.buffer(), recipient.bufsize());
    if (err < 0) {
      VERBOSE_ERR(m_opts, "Error in sendto: " << strerror(errno));
      return false;
    }
    return true;
  }



  virtual ssize_t recv(conn_index from_idx)
  {
    VERBOSE_LOG(m_opts, "Reading from " << from_idx);

    char buf[200] = { 0 };
    auto n = ::recv(m_conns[from_idx], buf, sizeof(buf), 0);
    if (n < 0 ) {
      VERBOSE_ERR(m_opts, "Error in recv:" << strerror(errno));
    }
    return n;
  }


  virtual void poll_events()
  {
    ev_run(m_loop, EVRUN_ONCE);
  }



  virtual void end_run()
  {
    for (conn_index i = 0 ; i < m_opts.conns; ++i) {
      ev_io_stop(m_loop, &watchers[i]);
    }

    contexts.clear();
  }

  options                                     m_opts;
  struct ev_loop *                            m_loop = EV_DEFAULT;
  std::vector<int>                            m_conns;
  std::vector<liberate::net::socket_address>  m_addrs;
};



static void
read_cb_bridge(EV_P_ ev_io * w, int revents)
{
  // Find the index of the watcher.
  size_t i = 0;
  for ( ; i < watchers.size() ; ++i) {
    if (&watchers[i] == w) {
      break;
    }
  }

  if (i >= watchers.size()) {
    std::cerr << "Nope, watcher not found." << std::endl;
    return;
  }

  callback_context & ctx = contexts[i];
  ctx.callback(*ctx.lops, i);
}



static const int _unused = (register_backend(backends::LIBEV,
      "libev", {"ev", "libev"},
      new libev_ops{}), 0);
