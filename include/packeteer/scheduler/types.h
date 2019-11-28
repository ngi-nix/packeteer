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
#ifndef PACKETEER_SCHEDULER_TYPES_H
#define PACKETEER_SCHEDULER_TYPES_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#include <chrono>

namespace PACKETEER_API packeteer {

/**
 * Types used in the scheduler, that may be of use elsewhere.
 **/
using duration = std::chrono::nanoseconds;
using clock    = std::chrono::steady_clock;

template <typename durationT = duration>
using clock_time_point = std::chrono::time_point<clock, durationT>;

using time_point = clock_time_point<duration>;


} // namespace packeteer

#endif // guard
