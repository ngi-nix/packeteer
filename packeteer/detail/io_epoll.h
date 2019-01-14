/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 * Copyright (c) 2012-2014 Unwesen Ltd.
 * Copyright (c) 2015-2019 Jens Finkhaeuser.
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
#ifndef PACKETEER_DETAIL_IO_EPOLL_H
#define PACKETEER_DETAIL_IO_EPOLL_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer/packeteer.h>

#if !defined(PACKETEER_HAVE_EPOLL_CREATE1)
#error epoll not detected
#endif

#include <packeteer/events.h>

#include <packeteer/detail/io.h>

namespace packeteer {
namespace detail {

// I/O subsystem based on epoll.
struct io_epoll : public io
{
public:
  io_epoll();
  ~io_epoll();

  void register_handle(handle const & handle, events_t const & events);
  void register_handles(handle const * handles, size_t amount, events_t const & events);

  void unregister_handle(handle const & handle, events_t const & events);
  void unregister_handles(handle const * handles, size_t amount, events_t const & events);

  virtual void wait_for_events(std::vector<event_data> & events,
      duration const & timeout);

private:
  /***************************************************************************
   * Data
   **/
  int       m_epoll_fd;
};


}} // namespace packeteer::detail

#endif // guard
