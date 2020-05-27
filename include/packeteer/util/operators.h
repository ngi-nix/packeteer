/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2017-2020 Jens Finkhaeuser.
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
#ifndef PACKETEER_UTIL_OPERATORS_H
#define PACKETEER_UTIL_OPERATORS_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

namespace packeteer::util {

/**
 * Supplement comparison operators when is_equal_to and is_less_than are
 * defined. Make sure to provide them as protected functions to allow class
 * hierarchies where every member uses this template.
 **/
	// FIXME not PACKETEER_API on windows, because of inline keyword. what about posix?
template <typename T>
struct operators
{
  inline bool operator==(T const & other) const
  {
    return static_cast<T const *>(this)->T::is_equal_to(other);
  }

  inline bool operator<(T const & other) const
  {
    return static_cast<T const *>(this)->T::is_less_than(other);
  }

  inline bool operator!=(T const & other) const
  {
    return !(*static_cast<T const *>(this) == other);
  }

  inline bool operator>(T const & other) const
  {
    return other < *static_cast<T const *>(this);
  }

  inline bool operator>=(T const & other) const
  {
    return !(*static_cast<T const *>(this) < other);
  }

  inline bool operator<=(T const & other) const
  {
    return !(other < *static_cast<T const *>(this));
  }
};


} // namespace packeteer::util

#endif // guard
