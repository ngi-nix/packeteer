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
}



void
io_win32::register_connector(connector const & conn, events_t const & events)
{
  connector conns[1] = { conn };
  register_connectors(conns, 1, events);
}



void
io_win32::register_connectors(connector const * conns, size_t size,
    events_t const & events)
{
  bool interrupt_select = false;

  for (size_t i = 0 ; i < size ; ++i) {
    connector const & conn = conns[i];
    DLOG("Registering connector " << conn << " for events " << events);

    if (handled_by_select(conn)) {
      m_select_thread->get_io()->register_connector(conn, events);
      interrupt_select = true;
    }
    else {
      m_iocp->register_connector(conn, events);
    }
  }

  if (interrupt_select) {
    m_select_thread->wakeup();
  }
}




void
io_win32::unregister_connector(connector const & conn, events_t const & events)
{
  connector conns[1] = { conn };
  unregister_connectors(conns, 1, events);
}




void
io_win32::unregister_connectors(connector const * conns, size_t size,
    events_t const & events)
{
  bool interrupt_select = false;

  for (size_t i = 0 ; i < size ; ++i) {
    connector const & conn = conns[i];
    DLOG("Unregistering connector " << conn << " from events " << events);

    if (handled_by_select(conn)) {
      m_select_thread->get_io()->unregister_connector(conn, events);
      interrupt_select = true;
    }
    else {
      m_iocp->unregister_connector(conn, events);
    }
  }

  if (interrupt_select) {
    m_select_thread->wakeup();
  }
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

  do {
    // Things are *fairly* simple here. Since we mainly use the IOCP loop and
    // treat the select loop as supplemental, we wait in the IOCP loop.
    m_iocp->wait_for_events(events, timeout);
    auto after = clock::now();
    auto tdiff = after - before;
    cur_timeout = timeout - tdiff;

    // However, we need to process the events the IOCP loop produced. If there
    // was a read event on our own queue_interrupt, we will likely have events
    // from the select loop.
    bool process_queue = false;
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
      }

      auto diff = events.size() - before;
      DLOG("Collected " << diff << " events from select loop.");
    }

  } while (events.empty() && cur_timeout > sc::milliseconds(1)); // IOCP resolution

  DLOG("WIN32 combined got " << events.size() << " event entries to report.");
}



} // namespace packeteer::detail
