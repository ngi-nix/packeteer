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

#include <packeteer/util/tmp.h>

#include "../macros.h"

#if defined(PACKETEER_WIN32)

#include "string.h"

namespace packeteer::util {

std::string
temp_name(std::string const & prefix /* = "" */)
{
  // Temporary directory
  wchar_t tmpdir[MAX_PATH + 1] = { 0 };
  auto len = GetTempPath(sizeof(tmpdir) / sizeof(wchar_t), tmpdir);
  if (len <= 0) {
    ERRNO_LOG("GetTempPath failed.");
    throw exception(ERR_UNEXPECTED, "GetTempPath failed.");
  }

  // Now get the temporary file name
  wchar_t buf[MAX_PATH + 1] = { 0 };
  auto wprefix = from_utf8(prefix.c_str());
  len = GetTempFileName(tmpdir, wprefix.c_str(), 0, buf);
  if ( len <= 0) {
    ERRNO_LOG("GetTempFileName failed.");
    throw exception(ERR_UNEXPECTED, "GetTempFileName failed.");
  }

  // Delete temporary file
  DeleteFile(buf);

  // Done
  return to_utf8(buf);
}

} // namespace packeteer::util

#else // PACKETEER_WIN32

#include <stdlib.h>
#include <unistd.h>

#include <vector>

namespace packeteer::util {

std::string
temp_name(std::string const & prefix /* = "" */)
{
  // Get temporary directory
  char const * tmpdir = getenv("TMPDIR");
  if (!tmpdir) {
    tmpdir = "/tmp";
  }

  // Template
  std::string templ = tmpdir;
  if (prefix.empty()) {
    templ += "/packeteer-XXXXXX";
  }
  else {
    templ += "/" + prefix + "-XXXXXX";
  }

  // Copy to writable buffer
  std::vector<char> buf{templ.begin(), templ.end()};
  buf.push_back(0);

  // Create temporary file
  int fd = mkstemp(&buf[0]);
  if (fd < 0) {
    ERRNO_LOG("mkstemp failed.");
    throw exception(ERR_UNEXPECTED, "mkstemp failed.");
  }
  std::string ret{buf.begin(), buf.end()};

  // Cleanup
  close(fd);
  unlink(ret.c_str());

  return ret;
}


} // namespace packeteer::util

#endif // PACKETEER_WIN32
