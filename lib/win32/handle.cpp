/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2019-2020 Jens Finkhaeuser.
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

#include <packeteer/handle.h>

#include "sys_handle.h"

namespace packeteer {

handle::sys_handle_t
handle::sys_make_dummy(size_t const & value)
{
  return std::make_shared<handle::opaque_handle>(reinterpret_cast<HANDLE>(value));
}



size_t
handle::sys_handle_hash(sys_handle_t const & handle)
{
  char const * p = reinterpret_cast<char const *>(&handle->handle);
  size_t state = std::hash<char>()(p[0]);
  for (size_t i = 1 ; i < sizeof(HANDLE) ; ++i) {
    packeteer::util::hash_combine(state, p[i]);
  }
  return state;
}



bool
handle::sys_equal(sys_handle_t const & first, sys_handle_t const & second)
{
  if (!first && !second) {
    return true;
  }
  if (!first || !second) {
    return false;
  }
  return first->handle == second->handle;
}


bool
handle::sys_less(sys_handle_t const & first, sys_handle_t const & second)
{
  if (!first && !second) {
    return false;
  }
  if (!first && second) {
    return true;
  }
  if (first && !second) {
    return false;
  }
  return first->handle < second->handle;
}

} // namespace packeteer
