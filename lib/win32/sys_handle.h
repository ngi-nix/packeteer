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
#ifndef PACKETEER_SYS_HANDLE_H
#define PAKCETEER_SYS_HANDLE_H

#include <packeteer/handle.h>

#include "../connector/win32/overlapped.h"

namespace packeteer {

/**
 * Because we map all I/O to overlapped functions, a handle is more than the
 * WIN32 HANDLE. It includes a blocking flag, so we can simulate blocking
 * operations. And it includes an overlapped::manager, for handling the
 * allocation and use of OVERLAPPED structures.
 */
struct handle::opaque_handle
{
  // Either HANDLE or SOCKET is used.
  union {
    HANDLE                      handle = INVALID_HANDLE_VALUE;
    SOCKET                      socket;
  };

  bool                          blocking = true;
  detail::overlapped::manager   overlapped_manager;

  opaque_handle(HANDLE h = INVALID_HANDLE_VALUE)
    : handle(h)
  {}

  opaque_handle(SOCKET s)
    : socket(s)
  {
  }
};

} // namespace packeteer

#endif // guard
