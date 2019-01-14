/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2014 Unwesen Ltd.
 * Copyright (c) 2015-2019 Jens Finkhaeuser.
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
#ifndef PACKETEER_THREAD_BINDER_H
#define PACKETEER_THREAD_BINDER_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer/packeteer.h>

#include <functional>

namespace packeteer {
namespace thread {

/**
 * Generates simple lambda wrappers for pointers to member functions.
 *
 * Overloaded for pointers, references, const and non-const.
 **/
template <
  typename T,
  typename retT,
  typename... argT
>
auto
binder(T const * t, retT (T::*func)(argT...) const)
{
  return [=](auto && ...args) -> retT {
    return (t->*func)(decltype(args)(args)...);
  };
}

template <
  typename T,
  typename retT,
  typename... argT
>
auto
binder(T * t, retT (T::*func)(argT...))
{
  return [=](auto && ...args) -> retT {
    return (t->*func)(decltype(args)(args)...);
  };
}

template <
  typename T,
  typename retT,
  typename... argT
>
auto
binder(T const & t, retT (T::*func)(argT...) const)
{
  return [=, &t](auto && ...args) -> retT {
    return (t.*func)(decltype(args)(args)...);
  };
}

template <
  typename T,
  typename retT,
  typename... argT
>
auto
binder(T & t, retT (T::*func)(argT...))
{
  return [=, &t](auto && ...args) -> retT {
    return (t.*func)(decltype(args)(args)...);
  };
}


}} // namespace packeteer::thread

#endif // guard
