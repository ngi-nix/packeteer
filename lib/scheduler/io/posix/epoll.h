/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 * Copyright (c) 2012-2014 Unwesen Ltd.
 * Copyright (c) 2015-2020 Jens Finkhaeuser.
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
#ifndef PACKETEER_SCHEDULER_IO_POSIX_EPOLL_H
#define PACKETEER_SCHEDULER_IO_POSIX_EPOLL_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#if !defined(PACKETEER_HAVE_EPOLL_CREATE1)
#error epoll not detected
#endif

#include <packeteer/scheduler/events.h>

#include "../../io.h"

namespace packeteer::detail {

// I/O subsystem based on epoll.
struct io_epoll : public io
{
public:
  explicit io_epoll(std::shared_ptr<api> api);
  ~io_epoll();

  void register_connector(connector const & conn, events_t const & events) override;
  void register_connectors(connector const * conns, size_t amount, events_t const & events) override;

  void unregister_connector(connector const & conn, events_t const & events) override;
  void unregister_connectors(connector const * conns, size_t amount, events_t const & events) override;

  virtual void wait_for_events(io_events & events,
      duration const & timeout) override;

private:
  /***************************************************************************
   * Data
   **/
  int m_epoll_fd;
};


} // namespace packeteer::detail

#endif // guard
