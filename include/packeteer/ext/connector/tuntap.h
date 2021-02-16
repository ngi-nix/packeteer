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
#ifndef PACKETEER_EXT_CONNECTOR_TUNTAP_H
#define PACKETEER_EXT_CONNECTOR_TUNTAP_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#include <packeteer/connector/types.h>

namespace packeteer::ext {

/**
 * FIXME
 */
PACKETEER_API
error_t
register_connector_tuntap(std::shared_ptr<packeteer::api> api,
    connector_type register_as = packeteer::CT_USER);

} // namespace packeteer::ext

#endif // guard
