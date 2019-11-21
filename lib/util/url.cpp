/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2018-2019 Jens Finkhaeuser.
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

#include <packeteer/util/url.h>

#include <iostream>
#include <algorithm>
#include <functional>
#include <locale>
#include <cwctype>

#include <packeteer/util/hash.h>

namespace packeteer::util {

namespace {

inline std::string::value_type
locale_lower(std::string::value_type const & c)
{
  return std::tolower<std::string::value_type>(c, std::locale());
}

std::string
normalize_value(std::string const & value)
{
  // std::cout << "pre normalization: " << value << std::endl;
  std::string ret;
  ret.resize(value.size());
  std::transform(value.begin(), value.end(), ret.begin(), locale_lower);

  if ("true" == ret or "yes" == ret) {
    ret = "1";
  } else if ("false" == ret or "no" == ret) {
    ret = "0";
  }

  return ret;
}

void
split_query(std::map<std::string, std::string> & params,
    std::string const & query)
{
  bool keep_going = true;
  std::string::size_type start = 0;
  do {
    auto end = query.find_first_of('&', start);
    if (std::string::npos == end) {
      end = query.length();
      keep_going = false;
    }
    // std::cout << "end at: " << end << std::endl;
    auto equal = query.find_first_of('=', start);
    if (std::string::npos == equal) {
      equal = query.length();
    }
    // std::cout << "equal at: " << equal << std::endl;
    if (equal >= end) {
      // Parameter without value.
      auto key = query.substr(start, end - start);
      std::transform(key.begin(), key.end(), key.begin(), locale_lower);

      // std::cout << "got simple: " << key << std::endl;
      params[key] = "1"; // Treat as boolean
      start = end + 1;
    }
    else {
      // Parameter with value
      auto key = query.substr(start, equal - start);
      std::transform(key.begin(), key.end(), key.begin(), locale_lower);

      auto value = normalize_value(query.substr(equal + 1, end - equal - 1));
      // std::cout << "got kv: " << key << " = " << value << std::endl;
      params[key] = value;
      start = end + 1;
    }
  } while (keep_going);
}

} // anonymous namespace


url
url::parse(char const * url_string)
{
  // Lazy... use the std::string version.
  return parse(std::string(url_string));
}


url
url::parse(std::string const & url_string)
{
  url ret;

  // We'll find the first occurrence of a colon; that *should* delimit the scheme.
  auto end = url_string.find_first_of(':');
  if (std::string::npos == end) {
    throw exception(ERR_FORMAT,
        std::string{"No scheme separator found in connector URL: "}
        + url_string);
  }
  // std::cout << "colon at: " << end << std::endl;

  // We'll then try to see if the characters immediately following are two
  // slashes.
  if (end + 3 > url_string.size()) {
    throw exception(ERR_FORMAT,
        std::string{"Bad scheme separator found in connector URL: "}
        + url_string);

  }
  // std::cout << "end + 1: " << url_string[end + 1] << std::endl;
  // std::cout << "end + 2: " << url_string[end + 2] << std::endl;
  if (!('/' == url_string[end + 1] && '/' == url_string[end + 2])) {
    throw exception(ERR_FORMAT,
        std::string{"Bad scheme separator found in connector URL: "}
        + url_string);
  }

  // Ok, we seem to have a scheme part. Lower-case it.
  ret.scheme = url_string.substr(0, end);
  std::transform(ret.scheme.begin(), ret.scheme.end(), ret.scheme.begin(),
      locale_lower);
  // std::cout << "got scheme: " << ret.scheme << std::endl;

  // Next grab the authority - that's everything until the next '/' character.
  auto start = end + 3;
  auto query_start = url_string.find_first_of('?', start);
  auto fragment_start = url_string.find_first_of('#', start);
  end = url_string.find_first_of('/', start);
  if (std::string::npos == end) {
    if (query_start != std::string::npos) {
      end = query_start;
    }
    else if (fragment_start != std::string::npos) {
      end = fragment_start;
    }
    else {
      end = url_string.length();
    }
  }
  // std::cout << "new end: " << end << std::endl;
  ret.authority = url_string.substr(start, end - start);
  // std::cout << "got authority: " << ret.authority << std::endl;

  // The path is anything up until a '?' or '#' or the end - whichever
  // comes first.
  start = end;
  if (query_start != std::string::npos) {
    end = query_start;
  }
  else if (fragment_start != std::string::npos) {
    end = fragment_start;
  }
  else {
    end = url_string.length();
  }
  // std::cout << "new end: " << end << std::endl;
  ret.path = url_string.substr(start, end - start);
  // std::cout << "got path: " << ret.path << std::endl;

  // The query is anything until the end or npos
  if (query_start != std::string::npos) {
    start = query_start + 1;
    end = fragment_start;
    if (std::string::npos == end) {
      end = url_string.length();
    }

    // std::cout << "new end: " << end << std::endl;
    auto query = url_string.substr(start, end - start);
    // std::cout << "got query: " << query << std::endl;
    split_query(ret.query, query);

    // for (auto elem : ret.query) {
    //   std::cout << "  " << elem.first << " = " << elem.second << std::endl;
    // }
  }

  // Last, the fragment
  if (fragment_start != std::string::npos) {
    ret.fragment = url_string.substr(fragment_start + 1);
    // std::cout << "got fragment: " << ret.fragment << std::endl;
  }

  return ret;
}



bool
url::is_equal_to(url const & other) const
{
  return scheme == other.scheme
    && authority == other.authority
    && path == other.path
    && query == other.query
    && fragment == other.fragment;
}


bool
url::is_less_than(url const & other) const
{
  return scheme < other.scheme
    || authority < other.authority
    || path < other.path
    || query < other.query
    || fragment < other.fragment;
}



std::string
url::str() const
{
  std::string ret = scheme + "://";
  ret += authority;
  ret += path;

  if (!query.empty()) {
    ret += "?";
    for (auto elem : query) {
      ret += elem.first + "=" + elem.second + "&";
    }
    ret.resize(ret.length() - 1);
  }

  if (!fragment.empty()) {
    ret += "#" + fragment;
  }

  return ret;
}



size_t
url::hash() const
{
  size_t base = packeteer::util::multi_hash(
      scheme, authority, path, fragment);

  for (auto elem : query) {
    packeteer::util::hash_combine(base,
        packeteer::util::multi_hash(elem.first, elem.second));
  }

  return base;
}


/*****************************************************************************
 * Friend functions
 **/
std::ostream &
operator<<(std::ostream & os, url const & data)
{
  os << data.str();
  return os;
}

} // namespace packeteer::util
