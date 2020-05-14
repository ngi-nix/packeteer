/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 * Copyright (c) 2012-2014 Unwesen Ltd.
 * Copyright (c) 2015-2020 Jens Finkhaeuser.
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

#include <packeteer/version.h>

#include "macros.h"

namespace packeteer {


std::pair<std::string, std::string>
version()
{
  return std::make_pair<std::string, std::string>(PACKETEER_PACKAGE_MAJOR,
      PACKETEER_PACKAGE_MINOR);
}



char const * copyright_string()
{
  return
    PACKETEER_PACKAGE_NAME " " PACKETEER_PACKAGE_VERSION " "
      PACKETEER_PACKAGE_URL "\n"
    "Copyright (c) 2011 Jens Finkhaeuser.\n"
    "Copyright (c) 2012-2014 Unwesen Ltd.\n"
    "Copyright (c) 2015-2020 Jens Finkhaeuser.\n"
    "Licensed under the the GPLv3 for personal, educational or non-profit "
    "use.\n"
    "Other licensing options available; please contact the copyright holder "
    "for information."
    ;
}


char const * license_string()
{
  return
    "This software is licensed under the terms of the GNU GPLv3 for personal,\n"
    "educational and non-profit use. For all other uses, alternative license\n"
    "options are available. Please contact the copyright holder for\n"
    "additional information, stating your intended usage.\n"
    "\n"
    "You can find the full text of the GPLv3 in the COPYING file in this code\n"
    "distribution.\n"
    "\n"
    "This software is distributed on an \"AS IS\" BASIS, WITHOUT ANY\n"
    "WARRANTY; without even the implied warranty of MERCHANTABILITY or\n"
    "FITNESS FOR A PARTICULAR PURPOSE."
    ;
}

} // namespace packeteer
