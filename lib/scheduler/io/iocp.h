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
#ifndef PACKETEER_SCHEDULER_IO_IOCP_H
#define PACKETEER_SCHEDULER_IO_IOCP_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#if !defined(PACKETEER_HAVE_IOCP)
#error I/O completion ports not detected
#endif

#include <set>

#include <packeteer/scheduler/events.h>

#include "../io.h"

namespace packeteer::detail {

// I/O subsystem based on select.
struct io_iocp : public io
{
public:
  io_iocp();
  ~io_iocp();

  void init();
  void deinit();

  virtual void
  register_connector(connector const & conn, events_t const & events);

  virtual void
  register_connectors(connector const * conns, size_t size,
      events_t const & events);

  virtual void
  unregister_connector(connector const & conn, events_t const & events);

  virtual void
  unregister_connectors(connector const * conns, size_t size,
      events_t const & events);

  virtual void wait_for_events(std::vector<event_data> & events,
      duration const & timeout);

private:
  using handle_key_t = size_t;

  /***************************************************************************
   * Data
   **/
  HANDLE                      m_iocp;
  std::set<HANDLE>            m_associated;
};


} // namespace packeteer::detail

#endif // guard
