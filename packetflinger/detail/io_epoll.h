/**
 * This file is part of packetflinger.
 *
 * Author(s): Jens Finkhaeuser <jens@unwesen.co.uk>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 * Copyright (c) 2012-2014 Unwesen Ltd.
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
#ifndef PACKETFLINGER_DETAIL_IO_EPOLL_H
#define PACKETFLINGER_DETAIL_IO_EPOLL_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packetflinger/packetflinger.h>

#if !defined(PACKETFLINGER_HAVE_EPOLL_CREATE1)
#error sys/epoll.h not detected
#endif

#include <packetflinger/events.h>

#include <packetflinger/detail/io.h>

namespace packetflinger {
namespace detail {

// I/O subsystem based on epoll.
struct io_epoll : public io
{
public:
  void init();
  void deinit();

  void register_fd(int fd, events_t const & events);
  void register_fds(int const * fds, size_t amount, events_t const & events);

  void unregister_fd(int fd, events_t const & events);
  void unregister_fds(int const * fds, size_t amount, events_t const & events);


private:
  /***************************************************************************
   * Data
   **/
  int       m_epoll_fd;
};


}} // namespace packetflinger::detail

#endif // guard
