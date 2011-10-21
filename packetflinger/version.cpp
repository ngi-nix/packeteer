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
    "Licensed under the Apache License, Version 2.0."
    ;


char const * const license_string =
    "Licensed under the Apache License, Version 2.0 (the \"License\");\n"
    "you may not use this file except in compliance with the License.\n"
    "You may obtain a copy of the License at\n"
    "\n"
    "    http://www.apache.org/licenses/LICENSE-2.0\n"
    "\n"
    "Unless required by applicable law or agreed to in writing, software\n"
    "distributed under the License is distributed on an \"AS IS\" BASIS,\n"
    "WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n"
    "See the License for the specific language governing permissions and\n"
    "limitations under the License."
    ;


} // namespace packetflinger
