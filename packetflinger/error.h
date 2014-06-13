/**
 * This file is part of packetflinger.
 *
 * Author(s): Jens Finkhaeuser <jens@unwesen.co.uk>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 * Copyright (c) 2012-2014 Unwesen Ltd.
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
#ifndef PACKETFLINGER_ERROR_H
#define PACKETFLINGER_ERROR_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packetflinger/packetflinger.h>

#include <stdexcept>

#include <packetflinger/macros.h>


/**
 * Macros for building a table of known error codes and associated messages.
 *
 * XXX Note that the macros contain namespace definitions, but are not expanded
 *     in a namespace scope. This is to allow files that include this header to
 *     decide what namespace/scope macros are to be expanded in.
 **/
#if !defined(PACKETFLINGER_START_ERRORS)
#define PACKETFLINGER_START_ERRORS namespace packetflinger { typedef enum {
#define PACKETFLINGER_ERRDEF(name, code, desc) name = code,
#define PACKETFLINGER_END_ERRORS PACKETFLINGER_ERROR_LAST } error_t; }
#define PACKETFLINGER_ERROR_FUNCTIONS
#endif


/*****************************************************************************
 * Error definitions
 **/
PACKETFLINGER_START_ERRORS

PACKETFLINGER_ERRDEF(ERR_SUCCESS,
    0,
    "No error")

PACKETFLINGER_ERRDEF(ERR_UNEXPECTED,
    1,
    "Nobody expects the Spanish Inquisition!")

PACKETFLINGER_ERRDEF(ERR_OUT_OF_MEMORY,
    2,
    "Out of memory.")

PACKETFLINGER_ERRDEF(ERR_ABORTED,
    3,
    "An operation was aborted due to unrecoverable errors.")

PACKETFLINGER_ERRDEF(ERR_FORMAT,
    4,
    "Invalid or unknown format specified.")

PACKETFLINGER_ERRDEF(ERR_INVALID_VALUE,
    5,
    "An invalid parameter value was specified.")

PACKETFLINGER_ERRDEF(ERR_NUM_FILES,
    6,
    "The system or per-user limit for open file descriptors was exceeded.")

PACKETFLINGER_ERRDEF(ERR_EMPTY_CALLBACK,
    7,
    "Tried to invoke an empty callback object.")

PACKETFLINGER_END_ERRORS


#if defined(PACKETFLINGER_ERROR_FUNCTIONS)

namespace packetflinger {

/*****************************************************************************
 * Functions
 **/

/**
 * Return the error message associated with the given error code. Never returns
 * nullptr; if an unknown error code is given, an "unidentified error" string is
 * returned. Not that this should happen, given that error_t is an enum...
 **/
char const * const error_message(error_t code);

/**
 * Return a string representation of the given error code. Also never returns
 * nullptr, see error_message() above.
 **/
char const * const error_name(error_t code);



/*****************************************************************************
 * Exception
 **/

/**
 * Exception class. Constructed with an error code and optional message;
 * wraps error_message() and error_name() above.
 **/
class exception : public std::runtime_error
{
public:
  /**
   * Constructor/destructor
   **/
  exception(error_t code, std::string const & details = std::string()) throw();
  exception(error_t code, int errnum);
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
  std::string const & details() const throw();

private:
  error_t     m_code;
  std::string m_details;
};


} // namespace packetflinger

#endif // PACKETFLINGER_ERROR_FUNCTIONS


/**
 * Undefine macros again
 **/
#undef PACKETFLINGER_START_ERRORS
#undef PACKETFLINGER_ERRDEF
#undef PACKETFLINGER_END_ERRORS

#undef PACKETFLINGER_ERROR_FUNCTIONS

#endif // guard
