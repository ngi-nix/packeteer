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
#ifndef PACKETEER_PACKETEER_H
#define PACKETEER_PACKETEER_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <memory>

/**
 * Which platform are we on?
 **/
#if !defined(PACKETEER_PLATFORM_DEFINED)
#  if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
#    define PACKETEER_WIN32
#  else
#    define PACKETEER_POSIX
#  endif
#  define PACKETEER_PLATFORM_DEFINED
#endif

// Visibility macros are used by all, so they must come first.
#include <packeteer/visibility.h>

// We want standard int types across the board.
#include <packeteer/types.h>

// Not all, but the very basic headers are always included.
#include <packeteer/error.h>
#include <packeteer/version.h>

/**
 * Decide what to include globally
 **/
#if defined(PACKETEER_WIN32)
// Include windows.h with minimal definitions
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#    define __UNDEF_LEAN_AND_MEAN
#  endif
#  define NOMINMAX
// Unicode builds
#  define UNICODE
#  define _UNICODE
#  include <windows.h>
#  include <WinDef.h>
#  ifdef __UNDEF_LEAN_AND_MEAN
#    undef WIN32_LEAN_AND_MEAN
#    undef __UNDEF_LEAN_AND_MEAN
#  endif
// Link against winsock2
#  pragma comment(lib, "Ws2_32.lib")
#endif


namespace packeteer {

/**
 * Forward
 */
class PACKETEER_API registry;


/**
 * The primary entry point into a packeteer library instance.
 */
class PACKETEER_API api
{
public:
  /**
   * Create a new API instance.
   */
  static std::shared_ptr<api> create()
  {
    return std::shared_ptr<api>(new api());
  }
  ~api();

  /**
   * Don't copy the instance - share it around, or create a new one.
   */
  api(api &&) = delete;
  api(api const &) = delete;
  api & operator=(api const &) = delete;

  /**
   * Access the registry interface.
   */
  registry & reg();

private:
  api();

  struct api_impl;
  std::unique_ptr<api_impl> m_impl;
};

} // namespace packeteer

#endif // guard
