/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2014 Unwesen Ltd.
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
#include <build-config.h>

#include "connector_url_params.h"

#include "macros.h"
#include "util/string.h"

namespace PACKETEER_API packeteer {
namespace detail {

namespace {

// We map url parameters to options mapping functions.
std::map<std::string, connector::option_mapping_function> option_mappers;

} // anonymous namespace


error_t
init_url_params()
{
  if (!option_mappers.empty()) {
    return ERR_SUCCESS;
  }

  LOG("Initializing default connector URL parameters.");
  error_t err = connector::register_option("behaviour",
      [](std::string const & value) -> connector_options
      {
        // Check for valid behaviour value
        connector_options requested = CO_DEFAULT;
        if ("datagram" == value or "dgram" == value) {
          requested = CO_DATAGRAM;
        } else if ("stream" == value) {
          requested = CO_STREAM;
        }
        return requested;
      }
  );
  if (ERR_SUCCESS != err) {
    return err;
  }

  err = connector::register_option("blocking",
      [](std::string const & value) -> connector_options
      {
        // Default to non-blocking options.
        if (value == "1") {
          return CO_BLOCKING;
        }
        return CO_NON_BLOCKING;
      }
  );
  if (ERR_SUCCESS != err) {
    return err;
  }


  // TODO others as needed.

  return ERR_SUCCESS;
}


error_t
register_url_param(std::string const & url_param,
    connector::option_mapping_function && mapper)
{
  if (url_param.empty()) {
    LOG("Must specify a URL parameter!");
    return ERR_INVALID_VALUE;
  }
  if (!mapper) {
    LOG("No mapper function provided!");
    return ERR_EMPTY_CALLBACK;
  }

  auto normalized = util::to_lower(url_param);

  auto option = option_mappers.find(normalized);
  if (option != option_mappers.end()) {
    LOG("URL parameter already registered!");
    return ERR_INVALID_VALUE;
  }

  // All good, so keep this.
  option_mappers[normalized] = std::move(mapper);

  return ERR_SUCCESS;

}


connector_options
options_from_url_params(std::map<std::string, std::string> const & query)
{
  connector_options result = CO_DEFAULT;
  for (auto param : option_mappers) {
    LOG("Checking known option parameter: " << param.first);

    // Get query value for parameter.
    auto query_val = query.find(param.first);
    std::string value;
    if (query_val != query.end()) {
      value = query_val->second;
    }

    // Invoke mapper
    LOG("Using mapper to convert value: " << value);
    auto intermediate = param.second(value);
    LOG("Mapper result is: " << intermediate);

    // Success, so set whatever the mapper set.
    result |= intermediate;
  }

  LOG("Merged options are: " << result);
  return result;
}


}} // namespace packeteer::detail

