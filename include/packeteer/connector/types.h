/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2014 Unwesen Ltd.
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
#ifndef PACKETEER_CONNECTOR_SPECS_H
#define PACKETEER_CONNECTOR_SPECS_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

namespace PACKETEER_API packeteer {

// Connector types can be any of the above constants, or a user-defined
// type. User-defined types must have type values of CT_USER or higher.
using connector_type = int16_t;

// Types of connectors. See connector.h for details, also on the naming scheme
// for peer addresses. Splitting into connector_type and the anonymous enum
// for constants enables people to e.g. use (CT_USER + 1), yet retain the
// underlying type.
enum PACKETEER_API : connector_type
{
  CT_UNSPEC = -1,
  CT_TCP4 = 0,
  CT_TCP6,
  CT_TCP,
  CT_UDP4,
  CT_UDP6,
  CT_UDP,
  CT_LOCAL,
  CT_PIPE,
  CT_ANON,
  CT_USER = 256, // First user-defined connector
};

// Connector behaviour & underlying type
using connector_options = uint16_t;

enum PACKETEER_API : connector_options
{
  CO_DEFAULT  = 0,            // typically the best pick

  CO_STREAM   = (1 << 0),     // STREAM connector; use read()/write()
  CO_DATAGRAM = (1 << 1),     // DATAGRAM connector; use receive()/send()

  CO_BLOCKING = (1 << 2),     // Blocking mode. Mutually exclusive with the below.
  CO_NON_BLOCKING = (1 << 3), // Non-blocking mode

  CO_USER     = (1 << 8),     // First user-defined options.
};


} // namespace packeteer

#endif // guard
