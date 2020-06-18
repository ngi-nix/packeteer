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


namespace packeteer::detail {


io_thread::io_thread(std::unique_ptr<io> io,
    connector io_interrupt,
    out_queue_t & out_queue,
    connector queue_interrupt)
  : m_io(std::move(io))
  , m_io_interrupt(io_interrupt)
  , m_out_queue(out_queue)
  , m_queue_interrupt(queue_interrupt)
{
  m_thread = std::thread{std::bind(&io_thread::thread_loop, this)};
}



void
io_thread::stop()
{
  m_running = false;
  interrupt(m_io_interrupt);
  m_thread.join();
}



void
io_thread::thread_loop()
{
  // Register I/O interrupt with I/O subsystem.
  m_io->register_connector(m_io_interrupt, PEV_IO_READ);

  DLOG("I/O loop started.");

  while (m_running) {
    // Wait for events. We want to wait forever until an event, aka a really
    // long time. But the I/O subsystem needs to be able to handle this.
    io_events events;
    m_io->wait_for_events(events, packeteer::duration::max());

    if (events.empty()) {
      continue;
    }

    DLOG("Got " << events.size() << " I/O events.");

    m_out_queue.push(events);
    interrupt(m_queue_interrupt);
  }

  DLOG("I/O loop ended, closing interrupt.");
  m_io->unregister_connector(m_io_interrupt, PEV_IO_READ);
  m_io_interrupt.close();
}



bool
io_thread::is_running() const
{
  return m_thread.joinable();
}


} // namespace packeteer::detail
