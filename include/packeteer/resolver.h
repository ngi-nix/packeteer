/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2020 Jens Finkhaeuser.
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
#ifndef PACKETEER_RESOLVER_H
#define PACKETEER_RESOLVER_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#include <functional>
#include <set>

#include <liberate/net/url.h>

namespace packeteer {

/**
 * The resolver class offers an interface for resolving URLs. In principle,
 * this is a little broader in scope than resolving solely host names to IP
 * addresses, though in practice, that is going to be the most used case.
 *
 * You can register resolving functions for URL schemes.
 *
 * A single input URL can resolve to multiple output URLs; for example,
 * resolving a host name can result in multiple IPv4 *and* IPv6 addresses.
 *
 * Note that the resolution functions may cache resolution results.
 */
class PACKETEER_API resolver
{
public:

  /**
   * Don't create or copy the resolver - access only through the api class.
   */
  ~resolver();

  resolver(resolver &&) = delete;
  resolver(resolver const &) = delete;
  resolver & operator=(resolver const &) = delete;

  /***************************************************************************
   * Resolver interface
   */
  /**
   * Register a new resolution function for a URL scheme.
   *
   * Returns:
   *  ERR_INVALID_VALUE if:
   *    - the url parameter is not specified or already registered
   *  ERR_EMPTY_CALLBACK if:
   *    - the mapper function is not specified.
   *
   * There is no deregistration; resolution functions are expected to be static
   * for the runtime of a program.
   *
   * The resolution function is taking a URL, and adding to a set of URLs. It
   * returns an error_t.
   *
   * The function is expected to return:
   *  - ERR_SUCCESS when it could resolve the URL successfully. This can mean
   *    that the result set contains only the unmodified URL, i.e. if the URL
   *    was already canonical.
   *  - ERR_INVALID_VALUE if the input URL was bad, for example if the part to
   *    resolve was missing.
   *  - Other error_t for resolution specific errors.
   */
  using resolution_function = std::function<
    error_t (
        api * api,
        std::set<liberate::net::url> &,
        liberate::net::url const &
    )
  >;

  error_t register_resolution_function(std::string const & scheme,
      resolution_function && resolution_func);


  /**
   * Resolves the query URL, and returns the result in the result set. The
   * result set is *not* cleared; this is up to the caller to do if desired.
   *
   * The function returns:
   * - ERR_SUCCES on success.
   * - ERR_INVALID_VALUE if no query was specified, or the query scheme was
   *   not recognised.
   * - Resolution function specific errors.
   *
   * Note that if the query URL was already canonical, the result set may
   * merely contain a copy of the query URL.
   */
  error_t resolve(std::set<liberate::net::url> & result,
      liberate::net::url const & query);

private:
  resolver(api * api);
  friend class api;

  struct resolver_impl;
  std::unique_ptr<resolver_impl> m_impl;
};

} // namespace packeteer

#endif // guard
