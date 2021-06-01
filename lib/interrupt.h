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
#ifndef PACKETEER_INTERRUPT_H
#define PACKETEER_INTERRUPT_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <build-config.h>

#include <packeteer/connector.h>

namespace packeteer::detail {

/**
 * Treat a connector as a signal, setting and clearing an interrupt by writing
 * to and reading from the connector.
 *
 * clear_interrupt() returns true if an interrupt was issued, false otherwise.
 **/
PACKETEER_PRIVATE
void set_interrupt(connector & signal);

PACKETEER_PRIVATE
bool clear_interrupt(connector & signal);

} // namespace packeteer::detail

#endif // guard
