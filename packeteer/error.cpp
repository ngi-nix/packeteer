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
#include <packeteer/error.h>

#include <cstring>

namespace packeteer {

/**
 * (Un-)define macros to ensure error defintions are being generated.
 **/
#define PACKETEER_START_ERRORS namespace { \
  static const struct { \
    char const * const name; \
    uint32_t code; \
    char const * const message; \
  } error_table[] = {
#define PACKETEER_ERRDEF(name, code, desc) \
  { PACKETEER_STRINGIFY(name), code, desc },
#define PACKETEER_END_ERRORS { NULL, 0, NULL } }; }

#undef PACKETEER_ERROR_H
#include <packeteer/error.h>



/*****************************************************************************
 * Functions
 **/


char const *
error_message(error_t code)
{
  if (0 > code || code >= PACKETEER_ERROR_LAST) {
    return "unidentified error";
  }

  for (uint32_t i = 0
      ; i < static_cast<error_t>(sizeof(error_table) / sizeof(error_table[0]))
      ; ++i)
  {
    if (static_cast<uint32_t>(code) == error_table[i].code) {
      return error_table[i].message;
    }
  }

  return "unidentified error";
}



char const *
error_name(error_t code)
{
  if (0 > code || code >= PACKETEER_ERROR_LAST) {
    return "unidentified error";
  }

  for (uint32_t i = 0
      ; i < static_cast<error_t>(sizeof(error_table) / sizeof(error_table[0]))
      ; ++i)
  {
    if (static_cast<uint32_t>(code) == error_table[i].code) {
      return error_table[i].name;
    }
  }

  return "unidentified error";
}


/*****************************************************************************
 * Exception
 **/
namespace {

// Helper function for trying to create a verbose error message. This may fail
// if there can't be any more allocations, for example, so be extra careful.
void combine_error(std::string & result, error_t code, int errnum, std::string const & details)
{
  try {
    result = "[" + std::string{error_name(code)} + "] ";
    result += std::string{error_message(code)};
    if (errnum) {
      result += " // ";
      result += ::strerror(errnum);
    }
    if (!details.empty()) {
      result += " // ";
      result += details;
    }
  } catch (...) {
  }
}

} // anonymous namespace


exception::exception(error_t code, std::string const & details /* = "" */) throw()
  : std::runtime_error("")
  , m_code(code)
{
  combine_error(m_message, m_code, 0, details);
}



exception::exception(error_t code, int errnum, std::string const & details /* = "" */) throw()
  : std::runtime_error("")
  , m_code(code)
{
  combine_error(m_message, m_code, errnum, details);
}




exception::~exception() throw()
{
}



char const *
exception::what() const throw()
{
  if (m_message.empty()) {
    return error_message(m_code);
  }
  return m_message.c_str();
}



char const *
exception::name() const throw()
{
  return error_name(m_code);
}



error_t
exception::code() const throw()
{
  return m_code;
}


} // namespace packeteer
