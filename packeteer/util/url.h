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
#ifndef PACKETEER_UTIL_URL_H
#define PACKETEER_UTIL_URL_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer/packeteer.h>
#include <packeteer/detail/operators.h>

#include <string>
#include <map>

#include <packeteer/error.h>

namespace packeteer {
namespace util {


/**
 * The URL class provides *minimal* URL parsing for specifying connector.
 *
 * That is, more advanced features of proper URLs are ignored - such as
 * using URL escaping, or quoting, etc. The main goal is to split a URL
 * into:
 * - scheme
 * - authority (host)
 * - path
 * - query parameters
 * - fragment
 *
 * The logic is simple enough that, for example, the first # found is
 * assumed to be the fragment delimiter, etc. It also does not deal with
 * duplicate parameters; the last found always wins.
 *
 * One thing that it does, however, is lower-case the scheme, all query
 * parameters and their values, and convert all values indicating
 * boolean values to simple "0" and "1" (e.g. "true", "yes", etc.).
 */
struct url
  : public ::packeteer::detail::operators<url>
{
public:
  // Data members
  std::string                         scheme;
  std::string                         authority;
  std::string                         path;
  std::map<std::string, std::string>  query;
  std::string                         fragment;

  // Static parse functions
  static url parse(char const * url_string);
  static url parse(std::string const & url_string);

  // Convert back to string
  std::string str() const;

  // Calculate hash
  size_t hash() const;

private:
  friend class packeteer::detail::operators<url>;

  bool is_less_than(url const & other) const;
  bool is_equal_to(url const & other) const;

  friend std::ostream & operator<<(std::ostream & os, url const & data);
};


/**
 * Formats a url into human-readable form.
 **/
std::ostream & operator<<(std::ostream & os, url const & data);


}} // namespace packeteer::util

/*******************************************************************************
 * std namespace specializations
 **/
namespace std {

template <> struct hash<::packeteer::util::url>
{
  size_t operator()(::packeteer::util::url const & x) const
  {
    return x.hash();
  }
};

} // namespace std


#endif // guard
