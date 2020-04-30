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

#include <bitset>

namespace packeteer::detail {

inline bool
add_behaviour(connector_options & result, connector_options const & behaviour)
{
  if (behaviour == CO_STREAM) {
    result |= CO_STREAM;
    result &= ~CO_DATAGRAM;
    // TODO result &= ~CO_SEQPACKET
    return true;
  }
  if (behaviour == CO_DATAGRAM) {
    result &= ~CO_STREAM;
    result |= CO_DATAGRAM;
    // TODO result &= ~CO_SEQPACKET
    return true;
  }
  // TODO if (behaviours == CO_SEQPACKET) ...

  return false;
}



inline connector_options
sanitize_options(connector_options const & input,
    connector_options const & defaults,
    connector_options const & behaviours)
{
  // Ensure behaviours has only valid values.
  auto check = behaviours;
  check &= ~CO_STREAM;
  check &= ~CO_DATAGRAM;
  // TODO check &= ~CO_SEQPACKET
  if (check || !behaviours) {
    throw exception(ERR_INVALID_VALUE, "Bad behaviour specified!");
  }

  // Start with defaults.
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

  // In single behaviour situations, we can just force the behaviour
  // and be done with it.
  if (add_behaviour(result, behaviours)) {
    // Was a single behaviour
    return result;
  }

  // Otherwise we're in a multi-behaviour situation, at which point
  // the input needs to contain the selected behaviour. Strip the input
  // down to only behaviours for understanding what is requested.
  auto behaviour = input;
  behaviour &= CO_STREAM | CO_DATAGRAM;

  if (!behaviour) {
    // The default should already be in the result.
    if (result & CO_STREAM || result & CO_DATAGRAM) {
      return result;
    }
    throw exception(ERR_INVALID_VALUE, "No behaviour selected, and no default behaviour found.");
  }

  if (!(behaviour & behaviours)) {
    throw exception(ERR_INVALID_VALUE, "Ambiguous or invalid behaviour selected!");
  }

  add_behaviour(result, behaviour);
  return result;
}

} // namespace packeteer::detail

#endif // guard
