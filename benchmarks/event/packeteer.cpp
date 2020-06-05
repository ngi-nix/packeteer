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

#include <packeteer.h>
#include <packeteer/error.h>
#include <packeteer/connector.h>
#include <packeteer/scheduler.h>
#include <packeteer/net/socket_address.h>

#include "backends.h"

using namespace packeteer;


struct p7r_ops : public backend_ops
{
  p7r_ops()
    : m_api{api::create()}
    , m_sched{m_api, 0}
  {
  }


  virtual ~p7r_ops()
  {
    for (conn_index i = 0 ; i < m_conns.size() ; ++i) {
      m_conns[i].close();
    }
  }



  virtual void init(options const & opts)
  {
    m_opts = opts;
    m_conns.resize(opts.conns);

    uint16_t port = opts.port_range_start;
    for (conn_index i = 0 ; i < opts.conns ; ++i, ++port) {
      net::socket_address addr{"127.0.0.1", port};
      auto url = util::url::parse("udp://" + addr.full_str());
      auto conn = connector(m_api, url);

      conn.listen();
      m_conns[i] = conn;
    }
  }


  virtual void start_run(read_callback callback)
  {
    for (conn_index i = 0 ; i < m_opts.conns; ++i) {
      m_sched.register_connector(PEV_IO_READ, m_conns[i],
          [this, callback, i](time_point const &, events_t, packeteer::error_t, connector *, void *) -> packeteer::error_t
          {
            callback(*this, i);
            return ERR_SUCCESS;
          }
      );
    }
  }



  virtual bool sendto(conn_index from_idx, conn_index to_idx, void const * buf, size_t buflen)
  {
    VERBOSE_LOG(m_opts, "Sending from " << from_idx << " to " << to_idx);

    auto recipient = m_conns[to_idx].socket_address();
    size_t bytes_written = 0;
    auto err = m_conns[from_idx].send(buf, buflen, bytes_written, recipient);
    if (err != ERR_SUCCESS && err != ERR_ASYNC) {
      VERBOSE_ERR(m_opts, "Error in send: " << error_name(err) << " / " << error_message(err));
      return false;
    }
    return true;
  }



  virtual ssize_t recv(conn_index from_idx)
  {
    VERBOSE_LOG(m_opts, "Reading from " << from_idx);

    char buf[200] = { 0 };
    size_t bytes_read = 0;
    peer_address sender;
    auto err = m_conns[from_idx].receive(buf, sizeof(buf), bytes_read, sender);
    if (err != ERR_SUCCESS && err != ERR_ASYNC) {
      VERBOSE_ERR(m_opts, "Error in receive: " << error_name(err) << " / " << error_message(err));
      return -1;
    }
    return bytes_read;
  }


  virtual void poll_events()
  {
    // Just use an insanely long timeout
    using namespace std::literals::chrono_literals;
    m_sched.process_events(24h);
  }



  virtual void end_run()
  {
    for (conn_index i = 0 ; i < m_opts.conns; ++i) {
      m_sched.unregister_connector(m_conns[i]);
    }
  }

  options                 m_opts;
  std::shared_ptr<api>    m_api;
  scheduler               m_sched;
  std::vector<connector>  m_conns;
};


static const int __unused = (register_backend(backends::PACKETEER,
      "packeteer", {"packeteer", "p7r"},
      new p7r_ops{}), 0);
