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
#include <packeteer.h>
#include <packeteer/detail/netincludes.h>

namespace packeteer {

#if defined(PACKETEER_WIN32)
bool init()
{
  WSADATA data;
  // Request Winsock 2.2
  int res = WSAStartup(MAKEWORD(2, 2), &data);
  return (res == 0);
}


void deinit()
{
  WSACleanup();
}

#else
bool init() { return true; }
void deinit() {}
#endif

} // namespace packeteer
