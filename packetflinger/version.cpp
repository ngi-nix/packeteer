/**
 * This file is part of packetflinger.
 *
 * Author(s): Jens Finkhaeuser <unwesen@users.sourceforge.net>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
    " - Copyright (c) 2011 Jens Finkhaeuser. Licensed under the Apache License,\n"
    "Version 2.0.\n"
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
    "limitations under the License.\n"
    ;


} // namespace packetflinger
