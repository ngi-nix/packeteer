/**
 * This file is part of packeteer.
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
#ifndef PACKETEER_EVENTS_H
#define PACKETEER_EVENTS_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer/packeteer.h>

namespace packeteer {

  // Event types for which callback functions can be registered. The callback
  // will be notified for which event(s) it was invoked.
  enum
  {
    PEV_IO_READ    = 1,     // A file descriptor is ready for reading
    PEV_IO_WRITE   = 2,     // A file descriptor is ready for writing
    PEV_IO_ERROR   = 4,     // A file descriptor has produced errors
    PEV_IO_CLOSE   = 8,     // A file descriptor has been closed. This event
                            // cannot be reliably reported.
    PEV_TIMEOUT    = 128,   // A timout has been reached that the callback was
                            // registered for.
    PEV_ERROR      = 256,   // Internal scheduler error.
    PEV_USER       = 32768, // A user-defined event was fired (see below).
  };

  typedef uint64_t events_t;


} // namespace packeteer

#endif // guard
