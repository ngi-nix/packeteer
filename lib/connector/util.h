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
#ifndef PACKETEER_CONNECTOR_UTIL_H
#define PACKETEER_CONNECTOR_UTIL_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <build-config.h>

namespace packeteer::detail {

connector_options
sanitize_options(connector_options const & input,
    connector_options const & defaults,
    connector_options const & type)
{
  connector_options result = defaults;

  // Set blocking/non-blocking if it's in the input.
  if (input & CO_BLOCKING) {
    result |= CO_BLOCKING;
    result &= ~CO_NON_BLOCKING;
  }
  else if (input & CO_NON_BLOCKING) {
    result &= ~CO_BLOCKING;
    result |= CO_NON_BLOCKING;
  }

  // Force type
  // TODO add CO_SEQPACKET if/when it happens
  switch (type) {
    case CO_STREAM:
      result |= CO_STREAM;
      result &= ~CO_DATAGRAM;
      break;

    case CO_DATAGRAM:
      result &= ~CO_STREAM;
      result |= CO_DATAGRAM;
      break;

    default:
      throw exception(ERR_INVALID_VALUE, "Type must always be either "
          "CO_DATAGRAM or CO_STREAM.");
  }

  return result;
}

} // namespace packeteer::detail

#endif // guard
