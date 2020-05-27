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
#include <packeteer.h>

#include <packeteer/registry.h>

#include "net/netincludes.h"

#include "macros.h"

namespace packeteer {

struct api::api_impl
{
  registry reg;
};



api::api()
  : m_impl{std::make_unique<api_impl>()}
{
#if defined(PACKETEER_WIN32)
  WSADATA data;
  // Request Winsock 2.2
  int res = WSAStartup(MAKEWORD(2, 2), &data);
  DLOG("WSA Description: " << data.szDescription);
  if (res != 0) {
    DLOG("WSA System Status: " << data.szSystemStatus);
    throw exception(ERR_INITIALIZATION, WSAGetLastError(),
        "WSAStartup failed!");
  }
#endif // win32
}



api::~api()
{
#if defined(PACKETEER_WIN32)
  WSACleanup();
  DLOG("WSA cleanup finished.");
#endif // win32
}



registry &
api::reg()
{
  return m_impl->reg;
}

} // namespace packeteer
