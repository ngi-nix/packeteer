/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2019 Jens Finkhaeuser.
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
#ifndef PACKETEER_CONNECTOR_CONNECTORS_H
#define PACKETEER_CONNECTOR_CONNECTORS_H

#include <build-config.h>

// POSIX vs Windows includes
#if defined(PACKETEER_POSIX)
#  include "posix/tcp.h"
#  include "posix/udp.h"
#  include "posix/local.h"
#else
// TODO windows includes
#endif

// Common includes
#include "connector/anon.h"
#include "connector/pipe.h"

#endif // guard
