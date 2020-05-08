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
#ifndef PACKETEER_SCHEDULER_IO_H
#define PACKETEER_SCHEDULER_IO_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#include <vector>
#include <chrono>
#include <map>

#include <packeteer/types.h>
#include <packeteer/connector.h>
#include <packeteer/scheduler/types.h>

namespace packeteer::detail {

/**
 * Forward declaration
 **/
struct event_data;

/**
 * Virtual base class for I/O subsystem. If you override the (un-)register*
 * functions, be sure to call the base implementation if you need m_sys_handles
 * to be updated.
 **/
struct io
{
public:
  io(std::shared_ptr<api> const & api)
    : m_api(api)
  {
  }

  virtual ~io() {};


  virtual void
  register_connector(connector const & conn, events_t const & events)
  {
    m_sys_handles[conn.get_read_handle().sys_handle()] |= events & ~PEV_IO_WRITE;
    m_sys_handles[conn.get_write_handle().sys_handle()] |= events & ~PEV_IO_READ;

    m_connectors[conn.get_read_handle().sys_handle()] = conn;
    m_connectors[conn.get_write_handle().sys_handle()] = conn;
  }



  virtual void
  register_connectors(connector const * conns, size_t size,
      events_t const & events)
  {
    for (size_t i = 0 ; i < size ; ++i) {
      m_sys_handles[conns[i].get_read_handle().sys_handle()] |= events & ~PEV_IO_WRITE;
      m_sys_handles[conns[i].get_write_handle().sys_handle()] |= events & ~PEV_IO_READ;

      m_connectors[conns[i].get_read_handle().sys_handle()] = conns[i];
      m_connectors[conns[i].get_write_handle().sys_handle()] = conns[i];
    }
  }



  virtual void
  unregister_connector(connector const & conn, events_t const & events)
  {
    clear_sys_handle_events(conn.get_read_handle().sys_handle(),
        events & ~PEV_IO_WRITE);
    clear_sys_handle_events(conn.get_write_handle().sys_handle(),
        events & ~PEV_IO_READ);
  }



  virtual void
  unregister_connectors(connector const * conns, size_t size,
      events_t const & events)
  {
    for (size_t i = 0 ; i < size ; ++i) {
      clear_sys_handle_events(conns[i].get_read_handle().sys_handle(),
          events & ~PEV_IO_WRITE);
      clear_sys_handle_events(conns[i].get_write_handle().sys_handle(),
          events & ~PEV_IO_READ);
    }
  }

  virtual void wait_for_events(std::vector<event_data> & events,
      packeteer::duration const & timeout) = 0;

protected:
  std::shared_ptr<api> m_api;

  std::map<handle::sys_handle_t, events_t>  m_sys_handles;
  std::map<handle::sys_handle_t, connector> m_connectors;

private:
  inline void
  clear_sys_handle_events(handle::sys_handle_t sys_handle, events_t const & events)
  {
    auto iter = m_sys_handles.find(sys_handle);
    if (iter == m_sys_handles.end()) {
      m_connectors.erase(sys_handle);
      return;
    }
    iter->second &= ~events;
    if (!iter->second) {
      m_sys_handles.erase(iter);
      m_connectors.erase(sys_handle);
    }
  }
};


} // namespace packeteer::detail

#endif // guard
