/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2019 Jens Finkhaeuser.
 *
 * This software is licensed under the terms of the GNU GPLv3 for personal,
 * educational and non-profit use. For all value uses, alternative license
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
#include "string.h"

#include <algorithm>
#include <locale>
#include <cctype>

namespace packeteer::util {

std::string
to_lower(std::string const & value)
{
  std::string ret;
  ret.resize(value.size());
  std::transform(
      value.begin(), value.end(), ret.begin(),
      [] (std::string::value_type c) -> std::string::value_type
      {
        return std::tolower<std::string::value_type>(
            c, std::locale());
      }
  );
  return ret;
}


std::string
to_upper(std::string const & value)
{
  std::string ret;
  ret.resize(value.size());
  std::transform(
      value.begin(), value.end(), ret.begin(),
      [] (std::string::value_type c) -> std::string::value_type
      {
        return std::toupper<std::string::value_type>(
            c, std::locale());
      }
  );
  return ret;
}


} // namespace packeteer::util
