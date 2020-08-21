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
#ifndef PACKETEER_ERROR_H
#define PACKETEER_ERROR_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#include <string>
#include <stdexcept>

/**
 * Macros for building a table of known error codes and associated messages.
 *
 * XXX Note that the macros contain namespace definitions, but are not expanded
 *     in a namespace scope. This is to allow files that include this header to
 *     decide what namespace/scope macros are to be expanded in.
 **/
#if !defined(PACKETEER_START_ERRORS)
#define PACKETEER_START_ERRORS \
  namespace packeteer { \
  using error_t = uint32_t; \
  enum PACKETEER_API : error_t {
#define PACKETEER_ERRDEF(name, code, desc) name = code,
#define PACKETEER_END_ERRORS \
    PACKETEER_ERROR_LAST, \
    PACKETEER_START_USER_RANGE = 1000 }; }
#define PACKETEER_ERROR_FUNCTIONS
#endif


/*****************************************************************************
 * Error definitions
 **/
PACKETEER_START_ERRORS

// Basic error codes
PACKETEER_ERRDEF(ERR_SUCCESS,
    0,
    "No error")

PACKETEER_ERRDEF(ERR_NOT_IMPLEMENTED,
    1,
    "An operation was attempted that is not implemented on this platform.")

PACKETEER_ERRDEF(ERR_INITIALIZATION,
    2,
    "An operation was attempted on an improperly initialized object.")

PACKETEER_ERRDEF(ERR_UNEXPECTED,
    3,
    "Nobody expects the Spanish Inquisition!")

PACKETEER_ERRDEF(ERR_ABORTED,
    4,
    "An operation was aborted due to unrecoverable errors.")

// Logic errors of some kind
PACKETEER_ERRDEF(ERR_UNSUPPORTED_ACTION,
    10,
    "The requested action was not supported for the object type.")

PACKETEER_ERRDEF(ERR_FORMAT,
    11,
    "Invalid or unknown format specified.")

PACKETEER_ERRDEF(ERR_INVALID_OPTION,
    12,
    "Invalid option specified.")

PACKETEER_ERRDEF(ERR_INVALID_VALUE,
    13,
    "An invalid parameter value was specified.")

// Asynchronous activity
PACKETEER_ERRDEF(ERR_REPEAT_ACTION,
    20,
    "The requested action would block or was interrupted and should be attempted again.")

PACKETEER_ERRDEF(ERR_ASYNC,
    21,
    "The requested action is being performed asynchronously.")

PACKETEER_ERRDEF(ERR_TIMEOUT,
    22,
    "A timeout occurred.")

// Internal errors
PACKETEER_ERRDEF(ERR_EMPTY_CALLBACK,
    30,
    "Tried to invoke an empty callback object.")

// Generic system errors
PACKETEER_ERRDEF(ERR_OUT_OF_MEMORY,
    40,
    "Out of memory.")

PACKETEER_ERRDEF(ERR_FS_ERROR,
    41,
    "File system error; this could be a nonexistent file name or a read only file system.")

PACKETEER_ERRDEF(ERR_NUM_FILES,
    42,
    "The system or per-user limit for open file descriptors was exceeded.")

PACKETEER_ERRDEF(ERR_NUM_ITEMS,
    43,
    "Too many items.")

PACKETEER_ERRDEF(ERR_ACCESS_VIOLATION,
    44,
    "An operation was attempted that was not permitted.")

// Networking errors
PACKETEER_ERRDEF(ERR_CONNECTION_REFUSED,
    50,
    "A connection was attempted but refused.")

PACKETEER_ERRDEF(ERR_CONNECTION_ABORTED,
    51,
    "A connection was aborted.")

PACKETEER_ERRDEF(ERR_NO_CONNECTION,
    52,
    "An operation was attempted that requires a connection, but no connection is established.")

PACKETEER_ERRDEF(ERR_NETWORK_UNREACHABLE,
    53,
    "Network is unreachable.")

PACKETEER_ERRDEF(ERR_ADDRESS_IN_USE,
    54,
    "Address is already in use.")

PACKETEER_ERRDEF(ERR_ADDRESS_NOT_AVAILABLE,
    55,
    "A nonexistent interface was requested or the requested address was not local.")

PACKETEER_END_ERRORS


#if defined(PACKETEER_ERROR_FUNCTIONS)

namespace packeteer {

/*****************************************************************************
 * Functions
 **/

/**
 * Return the error message associated with the given error code. Never returns
 * nullptr; if an unknown error code is given, an "unidentified error" string is
 * returned. Not that this should happen, given that error_t is an enum...
 **/
PACKETEER_API char const * error_message(error_t code);

/**
 * Return a string representation of the given error code. Also never returns
 * nullptr, see error_message() above.
 **/
PACKETEER_API char const * error_name(error_t code);



/*****************************************************************************
 * Exception
 **/

/**
 * Exception class. Constructed with an error code and optional message;
 * wraps error_message() and error_name() above.
 **/
class PACKETEER_API exception : public std::runtime_error
{
public:
  /**
   * Constructor/destructor
   **/
  explicit exception(error_t code, std::string const & details = std::string()) throw();
  exception(error_t code, int errnum, std::string const & details = std::string()) throw();
  virtual ~exception() throw();

  /**
   * std::exception interface
   **/
  virtual char const * what() const throw();

  /**
   * Additional functions
   **/
  char const * name() const throw();
  error_t code() const throw();

private:
  error_t     m_code;
  std::string m_message;
};


} // namespace packeteer

#endif // PACKETEER_ERROR_FUNCTIONS


/**
 * Undefine macros again
 **/
#undef PACKETEER_START_ERRORS
#undef PACKETEER_ERRDEF
#undef PACKETEER_END_ERRORS

#undef PACKETEER_ERROR_FUNCTIONS

#endif // guard
