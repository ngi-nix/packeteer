/**
 * This file is part of packetflinger.
 *
 * Author(s): Jens Finkhaeuser <unwesen@users.sourceforge.net>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **/
#ifndef PACKETFLINGER_DURATION_H
#define PACKETFLINGER_DURATION_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packetflinger/packetflinger.h>

#include <boost/limits.hpp>

namespace packetflinger {
namespace duration {

/*****************************************************************************
 * Time value with microsecond granularity.
 **/
typedef int64_t usec_t;


/*****************************************************************************
 * Time value conversions.
 **/

inline usec_t from_sec(int32_t sec)
{
  return static_cast<usec_t>(sec) * 1000000;
}


inline usec_t from_msec(int32_t msec)
{
  return static_cast<usec_t>(msec) * 1000;
}



inline int32_t to_msec(usec_t const & usec)
{
  return static_cast<int32_t>(usec / 1000);
}



inline int32_t to_sec(usec_t const & usec)
{
  return static_cast<int32_t>(usec / 1000000);
}



/*****************************************************************************
 * Utility functions
 **/

/**
 * Put the current thread to sleep for at least the specified interval. It's
 * possible that the kernel wakes the thread up a small amount after the
 * specified interval.
 *
 * May throw ERR_INVALID_VALUE, ERR_OUT_OF_MEMORY or ERR_UNEXPECTED in highly
 * unlikely circumstances (e.g. no kernel memory left, etc.).
 **/
void sleep(usec_t const & interval);



/**
 * Return the current system time, in microseconds.
 **/
usec_t now();

}} // namespace packetflinger::duration

#endif // guard