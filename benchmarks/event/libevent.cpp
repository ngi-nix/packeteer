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

#include <event.h>
#include <evutil.h>

#include <packeteer/net/socket_address.h>

#include "backends.h"

struct libevent_ops;

struct callback_context
{
  libevent_ops *  lops;
  conn_index          index;
  read_callback   callback;
};


static void
read_cb_bridge(evutil_socket_t fd, short which, void *arg);

struct libevent_ops : public backend_ops
{
  virtual ~libevent_ops()
  {
    for (conn_index i = 0 ; i < m_conns.size() ; ++i) {
      evutil_closesocket(m_conns[i]);
    }
  }



  virtual void init(options const & opts)
  {
    m_opts = opts;

    m_events.resize(opts.conns);
    m_conns.resize(opts.conns);
    m_addrs.resize(opts.conns);

#ifdef _WIN32
    WSADATA WSAData;
    WSAStartup(0x101, &WSAData);
#endif

    event_init();

    uint16_t port = opts.port_range_start;
    for (conn_index i = 0 ; i < opts.conns ; ++i, ++port) {
      auto h = socket(AF_INET, SOCK_DGRAM, 0);
      if (h == -1) {
        throw std::runtime_error("Could not create socket!");
      }
      evutil_make_socket_nonblocking(h);
      evutil_make_socket_closeonexec(h);

      packeteer::net::socket_address addr{"127.0.0.1", port};
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
    m_contexts.resize(m_opts.conns);

    for (conn_index i = 0 ; i < m_opts.conns; ++i) {
      m_contexts[i] = { this, i, callback };
      event_set(&m_events[i], m_conns[i], EV_READ | EV_PERSIST, read_cb_bridge, (void *) &m_contexts[i]);
      event_add(&m_events[i], NULL);
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
    event_loop(EVLOOP_ONCE | EVLOOP_NONBLOCK);
  }



  virtual void end_run()
  {
    for (conn_index i = 0 ; i < m_opts.conns; ++i) {
      if (event_initialized(&m_events[i])) {
        event_del(&m_events[i]);
      }
    }

    m_contexts.clear();
  }

  options                                     m_opts;
  std::vector<event>                          m_events;
  std::vector<evutil_socket_t>                m_conns;
  std::vector<packeteer::net::socket_address> m_addrs;
  std::vector<callback_context>               m_contexts;
};



static void
read_cb_bridge(evutil_socket_t fd, short which, void *arg)
{
  callback_context * ctx = (callback_context *) arg;
  ctx->callback(*(ctx->lops), ctx->index);
}



static const int __unused = (register_backend(backends::LIBEVENT,
      "libevent", {"event", "libevent"},
      new libevent_ops{}), 0);
