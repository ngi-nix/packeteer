/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 * Copyright (c) 2012-2014 Unwesen Ltd.
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
#ifndef PACKETEER_MACROS_H
#define PACKETEER_MACROS_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <sstream>

/**
 * Stringify the symbol passed to PACKETEER_SPRINGIFY()
 **/
#define PACKETEER_STRINGIFY(n) PACKETEER_STRINGIFY_HELPER(n)
#define PACKETEER_STRINGIFY_HELPER(n) #n

/**
 * Define logging symbols if -DDEBUG is specified (or -DNDEBUG isn't).
 *
 * Note: We can't use strerror_r() here because g++ turns on _GNU_SOURCE,
 *       which results in us getting the GNU version of the function rather
 *       than the POSIX version.
 **/
#if defined(DEBUG) && !defined(NDEBUG)
#include <iostream>
#include <string.h>
#define LOG(msg) std::cerr << "DEBUG: " << msg << std::endl;
#define ERRNO_LOG(msg) std::cerr << "DEBUG: " << msg << " // " \
  << ::strerror(errno) << std::endl;
#define ERR_LOG(msg, exc) std::cerr << "DEBUG: " << msg << " [" \
  << error_name(ex.code()) << "] " << ex.what() << " - " \
  << ex.details() << std::endl;
#else
#define LOG(msg)
#define ERRNO_LOG(msg)
#define ERR_LOG(msg, exc)
#endif

/**
 * Token concatenation
 **/
#define PACKETEER_TOKEN_CAT_HELPER(x, y) x ## y
#define PACKETEER_TOKEN_CAT(x, y) PACKETEER_TOKEN_CAT_HELPER(x, y)

/**
 * Simply macro for making an anonymous union from the given data structure/type,
 * and a cache line size'd padding value. Ensures the data is occupying a cache
 * line all by itself.
 **/
// FIXME not sure this is right, see cask
#define PACKETEER_CACHE_LINE_ALIGN(data)                                    \
  union {                                                                       \
    data;                                                                       \
    char PACKETEER_TOKEN_CAT(pad, __LINE__)[PACKETEER_CACHE_LINE_SIZE]; \
  };

#define PACKETEER_CACHE_LINE_PAD                                            \
  char PACKETEER_TOKEN_CAT(pad, __LINE__)[PACKETEER_CACHE_LINE_SIZE];

/**
 * Flow control guard; if this line is reached, an exception is thrown.
 **/
#define PACKETEER_FLOW_CONTROL_GUARD                                \
  {                                                                 \
    std::stringstream flowcontrol;                                  \
    flowcontrol << "Control should never have reached this line: "  \
      << __FILE__ << ":" << __LINE__;                               \
    throw exception(ERR_UNEXPECTED, flowcontrol.str());             \
  }

#endif // guard
