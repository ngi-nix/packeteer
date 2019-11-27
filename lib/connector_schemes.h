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
#ifndef PACKETEER_CONNECTOR_SCHEMES_H
#define PACKETEER_CONNECTOR_SCHEMES_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer/connector.h>

namespace PACKETEER_API packeteer {
namespace detail {

/**
 * Initialize scheme handlers - functions that create connector_interface
 * implementations.
 */
error_t init_schemes();

/**
 * Register a scheme.
 */
error_t register_scheme(std::string const & scheme,
    connector_type const & type,
    connector_options const & default_options,
    connector_options const & possible_options,
    connector::scheme_instantiation_function && creator);

/**
 * Find connector information for a given URL scheme.
 */
struct connector_info
{
  connector_type                            type;
  connector_options                         default_options;
  connector_options                         possible_options;
  connector::scheme_instantiation_function  creator;
};

connector_info
info_for_scheme(std::string const & scheme);


}} // namespace packeteer::detail

#endif // guard
