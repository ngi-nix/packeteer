/**
 * This file is part of packetflinger.
 *
 * Author(s): Jens Finkhaeuser <unwesen@users.sourceforge.net>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **/
#include <packetflinger/error.h>

namespace packetflinger {

/**
 * (Un-)define macros to ensure error defintions are being generated.
 **/
#define PACKETFLINGER_START_ERRORS namespace { static const struct { char const * const name; boost::uint32_t code; char const * const message; } error_table[] = {
#define PACKETFLINGER_ERRDEF(name, code, desc) { PACKETFLINGER_STRINGIFY(name), code, desc},
#define PACKETFLINGER_END_ERRORS { NULL, 0, NULL } }; }

#undef PACKETFLINGER_ERROR_H
#include <packetflinger/error.h>



/*****************************************************************************
 * Functions
 **/


char const * const
error_message(error_t code)
{
  if (0 > code || code >= PACKETFLINGER_ERROR_LAST) {
    return "unidentified error";
  }

  for (boost::uint32_t i = 0
      ; i < static_cast<error_t>(sizeof(error_table) / sizeof(error_table[0]))
      ; ++i)
  {
    if (static_cast<boost::uint32_t>(code) == error_table[i].code) {
      return error_table[i].message;
    }
  }

  return "unidentified error";
}



char const * const
error_name(error_t code)
{
  if (0 > code || code >= PACKETFLINGER_ERROR_LAST) {
    return "unidentified error";
  }

  for (boost::uint32_t i = 0
      ; i < static_cast<error_t>(sizeof(error_table) / sizeof(error_table[0]))
      ; ++i)
  {
    if (static_cast<boost::uint32_t>(code) == error_table[i].code) {
      return error_table[i].name;
    }
  }

  return "unidentified error";
}


/*****************************************************************************
 * Exception
 **/
exception::exception(error_t code, std::string const & details /* = "" */) throw()
  : std::runtime_error("")
  , m_code(code)
  , m_details(details)
{
}



exception::~exception() throw()
{
}



char const *
exception::what() const throw()
{
  return error_message(m_code);
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



std::string const &
exception::details() const throw()
{
  return m_details;
}


} // namespace packetflinger
