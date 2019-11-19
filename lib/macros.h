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

#include <build-config.h>

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
#define _LOG_BASE(severity, msg) { \
  std::stringstream s; \
  s << "[" << __FILE__ << ":" \
  << __LINE__ << "] " << severity << ": " << msg << std::endl; \
  std::cerr << s.str(); \
}
#define LOG(msg) _LOG_BASE("DEBUG", msg)
#define ERRNO_LOG(msg) _LOG_BASE("ERROR", msg << " // " \
  << ::strerror(errno))
#define ERR_LOG(msg, exc) _LOG_BASE("ERROR", msg << ex.what())
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
#define PACKETEER_SIZED_PAD(size) \
  char PACKETEER_TOKEN_CAT(pad, __LINE__)[size] // no semicolon,
                                                // leave to the caller

#define PACKETEER_CACHE_LINE_PAD \
  PACKETEER_SIZED_PAD(PACKETEER_CACHE_LINE_SIZE)

#define PACKETEER_CACHE_LINE_ALIGN(type, name) \
  union { \
    type name; \
    PACKETEER_CACHE_LINE_PAD; \
  } // no semicolon, leave to the caller.

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
