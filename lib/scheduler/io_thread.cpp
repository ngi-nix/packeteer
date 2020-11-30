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

#include "io_thread.h"

#include "scheduler_impl.h"

#include "../interrupt.h"


namespace packeteer::detail {


io_thread::io_thread(std::unique_ptr<io> io,
    connector io_interrupt,
    out_queue_t & out_queue,
    connector queue_interrupt,
    bool report_self /* = false */)
  : m_io(std::move(io))
  , m_io_interrupt(io_interrupt)
  , m_out_queue(out_queue)
  , m_queue_interrupt(queue_interrupt)
  , m_report_self(report_self)
{
}



io_thread::~io_thread()
{
  stop();
}



error_t
io_thread::start()
{
  if (is_running()) {
    return ERR_INVALID_VALUE;
  }

  try
  {
    m_thread = std::thread{std::bind(&io_thread::thread_loop, this)};
  } catch (packeteer::exception const & ex) {
    EXC_LOG("I/O thread start failed.", ex);
    m_error = std::current_exception();
    return ex.code();
  } catch (std::exception const & ex) {
    EXC_LOG("I/O thread start failed.", ex);
    m_error = std::current_exception();
    return ERR_UNEXPECTED;
  } catch (...) {
    ELOG("I/O thread start failed with unknown error.");
    m_error = std::make_exception_ptr(exception{ERR_UNEXPECTED, "Caught unknown exception in I/O thread start."});
    return ERR_UNEXPECTED;
  }
  return ERR_SUCCESS;
}



void
io_thread::wakeup()
{
  if (m_running) {
    set_interrupt(m_io_interrupt);
  }
}



void
io_thread::stop()
{
  bool was_running = m_running;
  m_running = false;

  if (was_running) {
    set_interrupt(m_io_interrupt);
  }

  if (m_thread.joinable()) {
    m_thread.join();
  }
}



std::exception_ptr
io_thread::error() const
{
  return m_error;
}



void
io_thread::register_connector(connector const & conn, events_t events)
{
  register_connectors(&conn, 1, events);
}



void
io_thread::register_connectors(connector const * conns, size_t amount,
    events_t events)
{
  for (size_t i = 0 ; i < amount ; ++i) {
    m_registration_queue.push({ REGISTER, conns[i], events});
  }
  wakeup();
}



void
io_thread::unregister_connector(connector const & conn, events_t events)
{
  unregister_connectors(&conn, 1, events);
}



void
io_thread::unregister_connectors(connector const * conns, size_t amount,
    events_t events)
{
  for (size_t i = 0 ; i < amount ; ++i) {
    m_registration_queue.push({ UNREGISTER, conns[i], events});
  }
  wakeup();
}



void
io_thread::thread_loop()
{
  try {
    // Register I/O interrupt with I/O subsystem.
    m_io->register_connector(m_io_interrupt, PEV_IO_READ);

    DLOG("I/O loop started: " << m_running);

    while (m_running) {
      // Before we wait for events, we need to process the registration queue.
      // For the sake of predictability, we do them in strict order, even if
      // that may not be very efficient.
      register_item item;
      while (m_registration_queue.pop(item)) {
        if (item.action == REGISTER) {
          m_io->register_connector(item.conn, item.events);
        }
        else if (item.action == UNREGISTER) {
          m_io->unregister_connector(item.conn, item.events);
        }
        else {
          PACKETEER_FLOW_CONTROL_GUARD;
        }
      }

      // Wait for events. We want to wait forever until an event, aka a really
      // long time. But the I/O subsystem needs to be able to handle this.
      io_events events;
      m_io->wait_for_events(events, packeteer::duration::max());

      if (events.empty()) {
        continue;
      }

      // If one of the events was to wake up the I/O loop, we have to clear
      // it. Unfortunately, we should also *remove* it, which is a little
      // more awkward because it means copying memory in the vector. But
      // so be it for now.
      if (!m_report_self) {
        for (auto iter = events.begin() ; iter != events.end() ; ++iter) {
          auto & ev = *iter;
          if (ev.connector == m_io_interrupt) {
            clear_interrupt(m_io_interrupt);
            events.erase(iter); // Only do that in the loop because we exit.
            break;
          }
        }
      }

      if (!events.empty()) {
        DLOG("Got " << events.size() << " I/O events.");
        m_out_queue.push(events);
        set_interrupt(m_queue_interrupt);
      }
    }

    DLOG("I/O loop ended, closing interrupt.");
    m_io->unregister_connector(m_io_interrupt, PEV_IO_READ);
    m_io_interrupt.close();

  } catch (packeteer::exception const & ex) {
    EXC_LOG("I/O thread loop failed.", ex);
    m_error = std::current_exception();
  } catch (std::exception const & ex) {
    EXC_LOG("I/O thread loop failed.", ex);
    m_error = std::current_exception();
  } catch (...) {
    ELOG("I/O thread loop failed with unknown error.");
    m_error = std::make_exception_ptr(exception{ERR_UNEXPECTED, "Caught unknown exception in I/O thread loop."});
  }

  // Thread is not running.
  m_running = false;
}



bool
io_thread::is_running() const
{
  return m_running && m_thread.joinable();
}


} // namespace packeteer::detail
