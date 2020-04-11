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
#ifndef PACKETEER_SCHEDULER_EVENTS_H
#define PACKETEER_SCHEDULER_EVENTS_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

namespace packeteer {

// Events are masked together into events_t. All event flags with values equal
// to or higher than PEV_USER are user-defined event types.
using events_t = uint32_t;


// Event types for which callback functions can be registered. The callback
// will be notified for which event(s) it was invoked.
enum PACKETEER_API : events_t
{
  PEV_IO_READ    = (1 <<  0),  // A handle is ready for reading
  PEV_IO_WRITE   = (1 <<  1),  // A handle is ready for writing
  PEV_IO_ERROR   = (1 <<  2),  // A handle has produced errors
  PEV_IO_OPEN    = (1 <<  3),  // A handle has been opened. This event only
                               // gets sent on some platforms.
  PEV_IO_CLOSE   = (1 <<  4),  // A handle has been closed. This event
                               // cannot be reliably reported, consider it
                               // informative only.

  PEV_TIMEOUT    = (1 <<  7),  // A timout has been reached that the callback was
                               // registered for.
  PEV_ERROR      = (1 <<  8),  // Internal scheduler error.

  PEV_USER       = (1 << 15), // A user-defined event was fired (see below).
};

} // namespace packeteer

#endif // guard
