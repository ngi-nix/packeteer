/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2019-2020 Jens Finkhaeuser.
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
#ifndef PACKETEER_TEST_TEST_NAME_H
#define PACKETEER_TEST_TEST_NAME_H

#define REPLACE_HELPER(c, replacement) \
  case c: \
    res += replacement; break ;

inline std::string
symbolize_name(std::string const & other)
{
  // std::cout << "NAME IS: >" << other << "< - " << other.size() << std::endl;
  std::string res;

  if (other.empty()) {
    res = "_empty_";
  }
  else {
    // Replace non-symbol with a sequence
    for (auto c : other) {
      switch (c) {
        REPLACE_HELPER('\0', "_null_");
        REPLACE_HELPER('.', "_dot_");
        REPLACE_HELPER(':', "_colon_");
        REPLACE_HELPER('/', "_slash_");
        REPLACE_HELPER('[', "_open_");
        REPLACE_HELPER(']', "_close_");
        REPLACE_HELPER(' ', "_");
        REPLACE_HELPER('\\', "_backslash_");
        REPLACE_HELPER('+', "_plus_");
        REPLACE_HELPER('-', "_minus_");
        REPLACE_HELPER('%', "_percent_");

        default:
          res += c;
          break;
      }
    }

    // Replace consecutive _ with a single
    auto len = res.length();
    for (size_t i = 0 ; i < len - 1 ; ++i) {
      if (res[i] != '_') continue;

      for (size_t j = i + 1 ; j < len - 1 ; ++j) {
        if (res[j] == '_') {
          res.erase(res.begin() + j);
          len = res.length();
        } else {
          break;
        }
      }
    }
  }

  return res;
}

#endif // guard
