/**
 * This file is part of packetflinger.
 *
 * Author(s): Jens Finkhaeuser <unwesen@users.sourceforge.net>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
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
#include <packetflinger/packetflinger-config.h>

#include <packetflinger/version.h>
#include <packetflinger/macros.h>

namespace packetflinger {


std::pair<std::string, std::string>
version()
{
  return std::make_pair<std::string, std::string>(PACKAGE_MAJOR, PACKAGE_MINOR);
}



char const * const copyright_string = PACKAGE_NAME " " PACKAGE_VERSION " " PACKAGE_URL
    " - Copyright (c) 2011 Jens Finkhaeuser.\n"
    "Licensed under the the GPLv3 for personal, educational or non-profit use.\n"
    "Other licensing options available; please contact the copyright holder for\n"
    "information."
    ;


char const * const license_string =

    "This software is licensed under the terms of the GNU GPLv3 for personal,\n"
    "educational and non-profit use. For all other uses, alternative license\n"
    "options are available. Please contact the copyright holder for additional\n"
    "information, stating your intended usage.\n"
    "\n"
    "You can find the full text of the GPLv3 in the COPYING file in this code\n"
    "distribution.\n"
    "\n"
    "This software is distributed on an \"AS IS\" BASIS, WITHOUT ANY WARRANTY;\n"
    "without even the implied warranty of MERCHANTABILITY or FITNESS FOR A\n"
    "PARTICULAR PURPOSE."
    ;


} // namespace packetflinger
