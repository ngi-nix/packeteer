/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2020 Jens Finkhaeuser.
 *
 * This software is licensed under the terms of the GNU GPLv3 for personal,
 * educational and non-profit use. For all value uses, alternative license
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
#include <packeteer/util/path.h>

#include <vector>

#include "string.h"

namespace packeteer::util {

namespace {

inline std::pair<bool, std::vector<std::string>>
tokenize(std::string const & input, std::string const & delim)
{
  std::vector<std::string> ret;
  bool leading_delim = false;

  std::string::size_type start = 0;
  std::string::size_type sub_start = 0;
  do {
    auto delim_pos = input.find(delim, start);

    // Remember if there was a leading delimiter
    if (delim_pos == 0) {
      leading_delim = true;
    }

    // Skip until the end.
    if (delim_pos == std::string::npos) {
      ret.push_back(input.substr(sub_start));
      break;
    }

    // Ignore if delimiter is quoted. We need to look *forward* for this on
    // WIN32, because they use a quote as a path separator, and it I can't even.
    if (delim == "\\") {
      if (delim_pos < input.size() - 2) {
        if (input[delim_pos + 1] != '\\') {
          ret.push_back(input.substr(sub_start, delim_pos - sub_start));
          start = delim_pos + delim.length();
          sub_start = start;
        }
        else {
          start = delim_pos + 1 + delim.length();
        }
      }
      else {
        ret.push_back(input.substr(sub_start, delim_pos - sub_start));
        start = delim_pos + delim.length();
        sub_start = start;
      }
    }
    else {
      if (delim_pos > 0 && input[delim_pos - 1] != '\\') {
        ret.push_back(input.substr(sub_start, delim_pos - sub_start));
      }
      start = delim_pos + delim.length();
      sub_start = start;
    }

  } while (start < input.length());

  return std::make_pair(leading_delim, ret);
}



inline std::string
join(std::vector<std::string> const & tokens, std::string const & delim)
{
  std::string ret;
  if (tokens.empty()) {
    return ret;
  }

  for (size_t idx = 0 ; idx < tokens.size() - 1 ; ++idx) {
    ret.append(tokens[idx]).append(delim);
  }

  return ret.append(tokens[tokens.size() - 1]);
}


} // anonymous namespace


std::string
to_posix_path(std::string const & other)
{
  auto token_data = tokenize(other, "\\");
  auto & tokens = token_data.second;

  // Drive letter manipulation
  if (!tokens.empty()) {
    auto & drive = tokens[0];
    // Seems like a valid drive; convert it. Otherwise it's a relative path,
    // and we don't do anything.
    if (drive.length() == 2 && drive[1] == ':') {
      auto lower = std::tolower(drive[0]);
      if (lower >= 'a' && lower <= 'z') {
        // Ok, this is a drive letter.
        drive[0] = '/';
        drive[1] = lower;
      }
      else {
        // Nope, this is weird.
        throw exception(ERR_INVALID_VALUE, "Input appears to contain a non-"
            "conforming drive letter!");
      }
    }
  }

  // Re-assemble tokens with correct delimiter.
  auto ret = join(tokens, "/");
  return ret;
}



std::string
to_win32_path(std::string const & other)
{
  auto token_data = tokenize(other, "/");
  auto & tokens = token_data.second;

  // Drive letter manipulation
  bool have_drive = false;
  if (token_data.first && !tokens.empty()) {
    auto & drive = tokens[0];
    if (drive.length() == 1) {
      auto upper = std::toupper(drive[0]);
      if (upper >= 'A' && upper <= 'Z') {
        // This *may* be a single-letter directory name at the root of the
        // current drive, but that's too hard to detect. So we'll assume it's
        // a drive.
        drive[0] = upper;
        drive += ":";
        have_drive = true;
      }
    }
  }

  // Any tokens that contain a "\\" must now have that quoted
  for (auto & tok : tokens) {
    // cppcheck-suppress useStlAlgorithm
    tok = replace(tok, "\\", "\\\\");
  }

  // Re-assemble tokens with correct delimiter.
  auto ret = join(tokens, "\\");
  if (token_data.first && !have_drive) {
    ret = "\\" + ret;
  }
  return ret;
}



} // namespace packeteer::util
