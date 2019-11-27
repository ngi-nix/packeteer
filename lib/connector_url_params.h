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
#ifndef PACKETEER_CONNECTOR_URL_PARAMS_H
#define PACKETEER_CONNECTOR_URL_PARAMS_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer/connector.h>

namespace PACKETEER_API packeteer {
namespace detail {

/**
 * Initialize default URL parameters. Return anything other than ERR_SUCCESS
 * on failure.
 */
error_t init_url_params();

/**
 * Register a parameter mapper function.
 */
error_t register_url_param(std::string const & url_param,
    connector::option_mapping_function && mapper);

/**
 * Given a map of string to string URL parameters, use the registered
 * mapping functions to return a merged options.
 */
connector_options
options_from_url_params(std::map<std::string, std::string> const & query);


}} // namespace packeteer::detail

#endif // guard
