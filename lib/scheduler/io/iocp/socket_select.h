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
#ifndef PACKETEER_SCHEDULER_IO_IOCP_SOCKET_SELECT_H
#define PACKETEER_SCHEDULER_IO_IOCP_SOCKET_SELECT_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#if !defined(PACKETEER_HAVE_IOCP)
#error I/O completion ports not detected
#endif

#include <mutex>
#include <thread>

#include <packeteer/scheduler/events.h>

#include "../iocp.h"
#include "../../../concurrent_queue.h"

namespace packeteer::detail {


class iocp_socket_select
{
public:
  struct socket_data
  {
    std::vector<handle::sys_handle_t> sockets;
    std::vector<WSAEVENT>             events;
  };

  struct result
  {
    handle::sys_handle_t  socket;
    events_t              events;
  };


  iocp_socket_select(connector & interrupt);
  ~iocp_socket_select();

  void configure_socket(handle::sys_handle_t const & sock, int events);

  concurrent_queue<result>  collected_events;

private:
  void run_loop();

  // Mutex protects the pointer
  std::mutex    m_mutex = {};
  socket_data * m_sock_data = nullptr;

  std::thread   m_thread = {};
  volatile bool m_running = true;

  connector &   m_interrupt;
};


} // namespace packeteer::detail

#endif // guard
