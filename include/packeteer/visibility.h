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

#if defined(_WIN32) || defined(__CYGWIN__) || defined(__MINGW32__)
  #ifdef PACKETEER_IS_BUILDING
    #define PACKETEER_API __declspec(dllexport)
  #else
    #define PACKETEER_API __declspec(dllimport)
  #endif
  #define PACKETEER_API_FRIEND PACKETEER_API
#else // Windows
  #if __GNUC__ >= 4
    #define PACKETEER_API  [[gnu::visibility("default")]]
  #else
    #define PACKETEER_API
  #endif // GNU C
  #define PACKETEER_API_FRIEND
#endif // POSIX

// Private symbols may be exported in debug builds for testing purposes.
#if defined(DEBUG)
  #define PACKETEER_PRIVATE PACKETEER_API
#else
  #define PACKETEER_PRIVATE
#endif

#endif // guard
