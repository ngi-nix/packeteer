/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2019-2020 Jens Finkhaeuser.
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
#include <sstream>
#include <iomanip>
#include <iostream> // FIXME

namespace packeteer::util {

std::string
to_lower(std::string const & value)
{
  std::string ret;
  ret.resize(value.size());
  std::transform(
      value.begin(), value.end(), ret.begin(),
      [] (std::string::value_type ch) -> std::string::value_type
      {
        return std::tolower<std::string::value_type>(
            ch, std::locale());
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
      [] (std::string::value_type ch) -> std::string::value_type
      {
        return std::toupper<std::string::value_type>(
            ch, std::locale());
      }
  );
  return ret;
}



std::string
replace(std::string const & haystack, std::string const & needle,
    std::string const & substitute, bool first_only /* = false */)
{
  std::string ret;

  std::string::size_type start = 0;
  do {
    auto delim_pos = haystack.find(needle, start);

    // Skip until the end.
    if (delim_pos == std::string::npos) {
      ret += haystack.substr(start);
      break;
    }

    // Take everything up until the needle, then add substitute
    ret += haystack.substr(start, delim_pos - start);
    ret += substitute;

    // Advance start by length of needle.
    start = delim_pos + needle.size();

    // If we only wanted the first match, bail out early.
    if (first_only) {
      ret += haystack.substr(start);
      break;
    }
  } while (start < haystack.length());

  return ret;
}


namespace {

//
// Templates case-insensitive equal
//
template <typename T>
struct case_insensitive_equal
{
  std::locale const & m_loc;

  inline explicit case_insensitive_equal(std::locale const & loc)
    : m_loc(loc)
  {
  }

  inline bool operator()(T const & first, T const & second)
  {
    return std::tolower(first, m_loc) == std::tolower(second, m_loc);
  }
};

} // anonymous namespace



ssize_t
ifind(std::string const & haystack, std::string const & needle)
{
  auto it = std::search(haystack.begin(), haystack.end(),
      needle.begin(), needle.end(),
      case_insensitive_equal<std::string::value_type>(std::locale()));
  if (it == haystack.end()) {
    return -1;
  }
  return it - haystack.begin();
}


#if defined(PACKETEER_WIN32)

std::string
to_utf8(TCHAR const * source)
{
  size_t size = WideCharToMultiByte(CP_UTF8, 0, source, -1, nullptr, 0,
      nullptr, nullptr);
  std::vector<char> buf(size, '\0');
  WideCharToMultiByte(CP_UTF8, 0, source, -1, &buf[0], size, nullptr, nullptr);
  return {buf.begin(), buf.end() - 1};
}



std::wstring
from_utf8(char const * source)
{
  size_t size = MultiByteToWideChar(CP_UTF8, 0, source, -1, nullptr, 0);
  std::vector<wchar_t> buf(size, 0);
  MultiByteToWideChar(CP_UTF8, 0, source, -1, &buf[0], size);
  return {buf.begin(), buf.end() - 1};
}


#endif // Win32



std::string
urlencode(std::string const & input)
{
  std::string ret;

  for (auto ch : input) {
    // Keep alphanumeric and other accepted characters intact
    if (isalnum(ch) || ch == '-' || ch == '_' || ch == '.' || ch == '/') {
      ret += ch;
      continue;
    }

    ret += "%";
    std::stringstream s;
    s.fill('0');
    s << std::hex << std::uppercase << std::setw(2)
      << static_cast<int>(static_cast<unsigned char>(ch));
    ret += s.str();
  }

  return ret;
}



std::string
urldecode(std::string const & input)
{
  std::string ret;

  for (std::string::size_type i = 0 ; i < input.size() ; ++i) {
    auto ch = input[i];

    // Percent-encoded
    if (ch == '%') {
      auto sub = input.substr(i + 1, 2);
      unsigned int val = 0;
      ::sscanf(sub.c_str(), "%x", &val);

      ret += static_cast<char>(val);
      i += 2;
      continue;
    }

    // Regular value
    ret += ch;
  }

  return ret;
}

} // namespace packeteer::util
