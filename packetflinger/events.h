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
#ifndef PACKETFLINGER_EVENTS_H
#define PACKETFLINGER_EVENTS_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packetflinger/packetflinger.h>

namespace packetflinger {

  // Event types for which callback functions can be registered. The callback
  // will be notified for which event(s) it was invoked.
  enum
  {
    EV_IO_READ    = 1,      // A file descriptor is ready for reading
    EV_IO_WRITE   = 2,      // A file descriptor is ready for writing
    EV_IO_ERROR   = 4,      // A file descriptor has produced errors
    EV_IO_CLOSE   = 8,      // A file descriptor has been closed.
    EV_TIMEOUT    = 128,    // A timout has been reached that the callback was
                            // registered for.
    EV_ERROR      = 256,    // Internal scheduler error.
    EV_USER       = 32768,  // A user-defined event was fired (see below).
  };

  typedef uint64_t events_t;


} // namespace packetflinger

#endif // guard
