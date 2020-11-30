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
#include <packeteer.h>

#include "interrupt.h"

namespace packeteer::detail {


void set_interrupt(connector & signal)
{
  char buf[1] = { '\0' };
  size_t amount = 0;
  signal.write(buf, sizeof(buf), amount);
}



bool clear_interrupt(connector & signal)
{
  char buf[1] = { '\0' };
  size_t amount = 0;
  signal.read(buf, sizeof(buf), amount);
  return (amount > 0);
}


} // namespace packeteer::detail
