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
#ifndef PACKETEER_SCHEDULER_IO_WIN32_WIN32_H
#define PACKETEER_SCHEDULER_IO_WIN32_WIN32_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#if !defined(PACKETEER_HAVE_IOCP)
#error I/O completion ports not detected
#endif

#include <unordered_set>

#include <packeteer/scheduler/events.h>

#include "../../io.h"
#include "../../io_thread.h"

namespace packeteer::detail {

// I/O subsystem combining io_iocp and io_select; the latter runs in an
// io_thread.
struct PACKETEER_PRIVATE io_win32 : public io
{
public:
  explicit io_win32(std::shared_ptr<api> const & api);
  ~io_win32();

  virtual void
  register_connector(connector const & conn, events_t const & events) override;

  virtual void
  register_connectors(connector const * conns, size_t size,
      events_t const & events) override;

  virtual void
  unregister_connector(connector const & conn, events_t const & events);

  virtual void
  unregister_connectors(connector const * conns, size_t size,
      events_t const & events) override;

  virtual void wait_for_events(io_events & events,
      duration const & timeout) override;

private:
  /***************************************************************************
   * Data
   **/
  std::unique_ptr<io> m_iocp;

  connector                   m_queue_interrupt;
  out_queue_t                 m_queue;
  std::unique_ptr<io_thread>  m_select_thread;
};


} // namespace packeteer::detail

#endif // guard
