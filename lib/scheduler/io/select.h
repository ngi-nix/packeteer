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
#ifndef PACKETEER_SCHEDULER_IO_SELECT_H
#define PACKETEER_SCHEDULER_IO_SELECT_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#if !defined(PACKETEER_HAVE_SELECT)
#error select not detected
#endif

#include <map>

#include <packeteer/scheduler/events.h>

#include "../io.h"

namespace packeteer::detail {

// I/O subsystem based on select.
struct io_select : public io
{
public:
  io_select();
  ~io_select();

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
  std::map<int, events_t> m_fds;
};


} // namespace packeteer::detail

#endif // guard
