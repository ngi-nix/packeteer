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
#include <build-config.h>

#include "win32.h"
#include "iocp.h"
#include "select.h"

#include "../../scheduler_impl.h"
#include "../../../win32/sys_handle.h"

namespace sc = std::chrono;

namespace packeteer::detail {

namespace {

inline bool
handled_by_select(connector const & conn)
{
  switch (conn.type()) {
    case CT_LOCAL:
    case CT_TCP:
    case CT_TCP4:
    case CT_TCP6:
    case CT_UDP:
    case CT_UDP4:
    case CT_UDP6:
      return true;

    default:
      return false;
  }
}

} // anonymous namespace


io_win32::io_win32(std::shared_ptr<api> const & api)
  : io{api}
  , m_iocp{new io_iocp{api}}
{
  // Create queue interrupt and select interrupt
  connector queue_interrupt{api, "anon://"};
  auto err = queue_interrupt.connect();
  if (ERR_SUCCESS != err) {
    throw exception{err, "Unable to create queue interrupt."};
  }

  connector select_interrupt{api, "local://"};
  err = select_interrupt.connect();
  if (ERR_SUCCESS != err) {
    throw exception{err, "Unable to create select interrupt."};
  }

  // Select I/O
  auto select = std::make_unique<io_select>(api);

  // Start thread
  auto th = std::make_unique<io_thread>(
    std::move(select),
    select_interrupt,
    m_queue,
    queue_interrupt);

  err = th->start();
  if (ERR_SUCCESS != err) {
    throw exception{err, "Unable to start select I/O thread."};
  }

  // All good? Then keep the temporaries.
  m_queue_interrupt = queue_interrupt;
  m_select_thread = std::move(th);

  // Register queue interrupt with IOCP
  m_iocp->register_connector(m_queue_interrupt, PEV_IO_READ);
}



io_win32::~io_win32()
{
  DLOG("Shutting down WIN32 I/O.");
  m_select_thread->stop();
}



void
io_win32::register_connector(connector const & conn, events_t const & events)
{
  connector conns[1] = { conn };
  register_connectors(conns, 1, events);
}



void
io_win32::register_connectors(connector const * conns, size_t amount,
    events_t const & events)
{
  std::vector<connector> iocp_conns;
  std::vector<connector> select_conns;
  for (size_t i = 0 ; i < amount ; ++i) {
    connector const & conn = conns[i];
    DLOG("Registering connector " << conn << " for events " << events);

    if (handled_by_select(conn)) {
      select_conns.push_back(conn);
    }
    else {
      iocp_conns.push_back(conn);
    }
  }

  if (!iocp_conns.empty()) {
    DLOG("Registering " << iocp_conns.size() << " IOCP connectors.");
    m_iocp->register_connectors(&iocp_conns[0], iocp_conns.size(), events);
  }

  if (!select_conns.empty()) {
    DLOG("Registering " << select_conns.size() << " select connectors.");
    m_select_thread->register_connectors(&select_conns[0], select_conns.size(),
        events);
  }

  io::register_connectors(conns, amount, events);
}




void
io_win32::unregister_connector(connector const & conn, events_t const & events)
{
  connector conns[1] = { conn };
  unregister_connectors(conns, 1, events);
}




void
io_win32::unregister_connectors(connector const * conns, size_t amount,
    events_t const & events)
{
  std::vector<connector> iocp_conns;
  std::vector<connector> select_conns;
  for (size_t i = 0 ; i < amount ; ++i) {
    connector const & conn = conns[i];
    DLOG("Unregistering connector " << conn << " from events " << events);

    if (handled_by_select(conn)) {
      select_conns.push_back(conn);
    }
    else {
      iocp_conns.push_back(conn);
    }
  }

  if (!iocp_conns.empty()) {
    DLOG("Unregistering " << iocp_conns.size() << " IOCP connectors.");
    m_iocp->unregister_connectors(&iocp_conns[0], iocp_conns.size(), events);
  }

  if (!select_conns.empty()) {
    DLOG("Unregistering " << select_conns.size() << " select connectors.");
    m_select_thread->unregister_connectors(&select_conns[0], select_conns.size(),
        events);
  }

  io::unregister_connectors(conns, amount, events);
}



void
io_win32::wait_for_events(io_events & events,
      duration const & timeout)
{
  // The main complication in this aggregated I/O subsystem is that if we
  // got woken *without* events, which happens if there were only internal
  // events, then we have to retry our sleep time.
  auto before = clock::now();
  auto cur_timeout = timeout;

  bool process_queue = false;
  do {
    // Things are *fairly* simple here. Since we mainly use the IOCP loop and
    // treat the select loop as supplemental, we wait in the IOCP loop.
    m_iocp->wait_for_events(events, cur_timeout);
    auto after = clock::now();
    auto tdiff = after - before;
    cur_timeout = timeout - tdiff;

    // However, we need to process the events the IOCP loop produced. If there
    // was a read event on our own queue_interrupt, we will likely have events
    // from the select loop.
    process_queue = false;
    for (auto iter = events.begin() ; iter != events.end() ; ++iter) {
      auto & ev = *iter;
      if (ev.connector == m_queue_interrupt) {
        process_queue = true;
        clear_interrupt(m_queue_interrupt);
        events.erase(iter); // Only do that because we exit the loop here.
        break;
      }
    }

    if (process_queue) {
      auto before = events.size();
      io_events from_select;
      while (m_queue.pop(from_select)) {
        events.insert(std::end(events), std::begin(from_select),
            std::end(from_select));

        // If any of the handles we got from select have pending reads, we can
        // move them from select to IOCP. The reason we use select in the first
        // place is that unlike with Windows pipes, sockets cannot have pending
        // read before being connected somehow. So we wait for select to signal
        // the connector as readable, at which point IOCP can take over. But
        // IOCP can only take over if there's a read pending, so we wait for that
        // situation.
        // Note: removing and re-adding these connectors will put them into the
        // select loop again; there is no logic to prevent that at this point.
        // It *is* possible, though, because pending reads can happen even
        // before being added to the scheduler.
        // Track changes in https://gitlab.com/interpeer/packeteer/-/issues/20
        std::vector<connector> conns;
        for (auto & [conn, ev] : from_select) {
          if (conn.get_read_handle().sys_handle()->read_context.pending_io()) {
            conns.push_back(conn);
          }
        }
        if (!conns.empty()) {
          DLOG("Select connetors with pending reads:" << conns.size());

          m_select_thread->unregister_connectors(&conns[0], conns.size(),
              PEV_IO_READ|PEV_IO_WRITE);

          for (auto & conn : conns) {
            events_t ev = m_sys_handles[conn.get_read_handle().sys_handle()];
            ev |= m_sys_handles[conn.get_write_handle().sys_handle()];
            m_iocp->register_connector(conn, ev);
          }
        }
      }

      auto diff = events.size() - before;
      DLOG("Collected " << diff << " events from select loop.");
    }

  } while (events.empty() && !process_queue && cur_timeout > sc::milliseconds(1)); // IOCP resolution

  DLOG("WIN32 combined got " << events.size() << " event entries to report.");
}



} // namespace packeteer::detail
