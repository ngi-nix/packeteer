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

#ifndef PACKETEER_THREAD_CHRONO_H
#define PACKETEER_THREAD_CHRONO_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#ifdef PACKETEER_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef PACKETEER_HAVE_TIME_H
#include <time.h>
#endif

#include <chrono>

#include <packeteer/error.h>

namespace packeteer::thread::chrono {

template <typename inT, typename outT>
inline void convert(inT const &, outT &)
{
  throw packeteer::exception(ERR_UNEXPECTED, "Conversion from "
      "std::chrono::duration to selected type not implemented.!");
}

#ifdef PACKETEER_HAVE_SYS_TIME_H
template <typename inT>
inline void convert(inT const & in, ::timeval & val)
{
  auto repr = ::std::chrono::round<::std::chrono::microseconds>(in).count();

  val.tv_sec = time_t(repr / 1'000'000);
  val.tv_usec = repr % 1'000'000;
}
#endif


#ifdef PACKETEER_HAVE_TIME_H
template <typename inT>
inline void convert(inT const & in, ::timespec & val)
{
  auto repr = ::std::chrono::round<::std::chrono::nanoseconds>(in).count();

  val.tv_sec = time_t(repr / 1'000'000'000);
  val.tv_nsec = repr % 1'000'000'000;
}
#endif


} // namespace packeteer::thread::chrono

#endif // guard
