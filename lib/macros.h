/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 * Copyright (c) 2012-2014 Unwesen Ltd.
 * Copyright (c) 2015-2020 Jens Finkhaeuser.
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
#include <iomanip>

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
#include <liberate/logging.h>
#define DLOG(msg) LIBLOG_DEBUG(msg)
#define ELOG(msg) LIBLOG_ERROR(msg)

#define EXC_LOG(msg, exc) LIBLOG_EXC(exc, msg)
#define ERR_LOG(msg, code) LIBLOG_ERR(code, msg)
#define ERRNO_LOG(msg) LIBLOG_ERRNO(msg)

// error_t logging
#define ET_LOG(msg, code) LIBLOG_ERROR(msg << " // " << error_name(code) << ": " \
    << error_message(code));

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
  char PACKETEER_TOKEN_CAT(pad, __LINE__)[size] = {}  // no semicolon,
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
#define PACKETEER_FLOW_CONTROL_GUARD_WITH(msg)                      \
  {                                                                 \
    std::stringstream flowcontrol;                                  \
    std::string mess{msg};                                          \
    if (!mess.empty()) {                                            \
      flowcontrol << mess << " - ";                                 \
    }                                                               \
    flowcontrol << "Control should never have reached this line: "  \
      << __FILE__ << ":" << __LINE__;                               \
    throw exception(ERR_UNEXPECTED, flowcontrol.str());             \
  }

#define PACKETEER_FLOW_CONTROL_GUARD PACKETEER_FLOW_CONTROL_GUARD_WITH("")


/**
 * Support for clang/OCLint suppressions
 **/
#if defined(__clang__) and defined(OCLINT_IS_RUNNING)
#  define OCLINT_SUPPRESS(suppression) \
    __attribute__(( \
      annotate("oclint:suppress[" suppression "]") \
    ))
#else
#  define OCLINT_SUPPRESS(annotation)
#endif

#endif // guard
