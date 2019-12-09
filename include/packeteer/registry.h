/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2019 Jens Finkhaeuser.
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
#ifndef PACKETEER_REGISTRY_H
#define PACKETEER_REGISTRY_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#include <functional>
#include <map>

#include <packeteer/connector/types.h>
#include <packeteer/connector/interface.h>

#include <packeteer/util/url.h>

namespace packeteer {

/**
 * The registry class offers an interface for registering extensions with
 * the API instance. Currently, there are two types of extensions:
 *
 *   1) Map a connector URL scheme to a connector implementation.
 *   1) Map a connector URL query parameter to a combination of
 *      CO_* options.
 */
class PACKETEER_API registry
{
public:

  /**
   * Don't create or copy the registry - access only through the api class.
   */
  ~registry();

  registry(registry &&) = delete;
  registry(registry const &) = delete;
  registry & operator=(registry const &) = delete;

  /***************************************************************************
   * Option parameter interface.
   */
  /**
   * Register a new query parameter function for connector options.
   *
   * Returns:
   *  ERR_INVALID_VALUE if:
   *    - the url parameter is not specified or already registered
   *  ERR_EMPTY_CALLBACK if:
   *    - the mapper function is not specified.
   *
   * Note that built-in option parameters count as already registered.
   *
   * There is no deregistration; options are expected to be static for
   * the runtime of a program.
   *
   * The mapper function is taking a string value of a URL parameter, and
   * returns a corresponding connector_options value. It also takes a boolean
   * flag indicating whether the parameter was found at all, allowing to
   * differentiate between a parameter without value and a missing parameter.
   *
   * A mapper may return any valid combination of options.
   */
  using option_mapper = std::function<
    ::packeteer::connector_options (std::string const &, bool)
  >;

  /**
   * Add a new parameter to the known list.
   */
  error_t add_parameter(std::string const & parameter,
      option_mapper && mapper);

  /**
   * Return the known connector_options from the set of query parameter
   * key/values provided.
   */
  connector_options options_from_query(
      std::map<std::string, std::string> const & query);

  /***************************************************************************
   * Scheme interface.
   */
  /**
   * Register a new connector scheme, i.e. a subclass of connector_interface.
   *
   * - The scheme string must be unique.
   * - The connector_type of the connector scheme must be provided. While it
   *   does not have to be unique, that would usually be the case.
   * - The default_options are provided to the creator function if no other
   *   options are specified during creation.
   * - The possible_options are verified; a connector with the given scheme
   *   and mismatching options cannot be created.
   * - The creator function.
   *
   * The creator function is passed a URL, the type associated with the scheme,
   * and parsed & validated options. It should raise exceptions if it cannot
   * create an instance. If it returns a nullptr, an exception is raised instead.
   */
  using scheme_creator = std::function<
    ::packeteer::connector_interface * (
        util::url const &,
        connector_type const & type,
        connector_options const &
    )
  >;

  /**
   * The information that's stored and returned in the registry.
   */
  struct connector_info
  {
    connector_type    type;
    connector_options default_options;
    connector_options possible_options;
    scheme_creator    creator;
  };

  /**
   * Add new connector info.
   */
  error_t
  add_scheme(std::string const & scheme, connector_info const & info);

  error_t
  add_scheme(std::string const & scheme,
      connector_type const & type,
      connector_options const & default_options,
      connector_options const & possible_options,
      scheme_creator && creator)
  {
    return add_scheme(scheme, connector_info{
        type, default_options, possible_options,
        std::move(creator)});
  }


  /**
   * Return stored scheme information.
   */
  connector_info info_for_scheme(std::string const & scheme);

private:
  registry();
  friend class api;

  struct registry_impl;
  PACKETEER_PROPAGATE_CONST(std::unique_ptr<registry_impl>) m_impl;
};

} // namespace packeteer

#endif // guard
