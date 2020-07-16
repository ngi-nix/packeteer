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
#ifndef PACKETEER_VISIBILITY_H
#define PACKETEER_VISIBILITY_H

/**
 * Use liberate's macros. To define them right, set LIBERATE_IS_BUILDING to
 * the value we've got from our own meson.build before including its
 * visibility header. Then restore the previous definition again, just in
 * case.
 */
#define _LIBERATE_IS_BUILDING LIBERATE_IS_BUILDING
#define LIBERATE_IS_BUILDING PACKETEER_IS_BUILDING

#include <liberate/visibility.h>

#undef LIBERATE_IS_BUILDING
#define LIBERATE_IS_BUILDING _LIBERATE_IS_BUILDING

/**
 * TODO: we can replace the PACKETEER_* macros across the library at some point,
 *       or leave these aliases.
 */
#define PACKETEER_API LIBERATE_API
#define PACKETEER_API_FRIEND LIBERATE_API_FRIEND
#define PACKETEER_PRIVATE LIBERATE_PRIVATE

#endif // guard
