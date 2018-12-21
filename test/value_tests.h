/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2017-2019 Jens Finkhaeuser.
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

#include <cppunit/extensions/HelperMacros.h>

#include <functional>

/**
 * Simple test case for value types. We expect at least two values to be
 * provided here, one "smaller", one "larger".
 **/
template <typename T>
inline T
testValues(T const & smaller, T const & larger, bool equal)
{
  // Operators
  if (equal) {
    CPPUNIT_ASSERT(!(smaller < larger));
    CPPUNIT_ASSERT(!std::less<T>()(smaller, larger));

    CPPUNIT_ASSERT(smaller <= larger);
    CPPUNIT_ASSERT(std::less_equal<T>()(smaller, larger));

    CPPUNIT_ASSERT(!(larger > smaller));
    CPPUNIT_ASSERT(!std::greater<T>()(larger, smaller));

    CPPUNIT_ASSERT(larger >= smaller);
    CPPUNIT_ASSERT(std::greater_equal<T>()(larger, smaller));

    CPPUNIT_ASSERT(smaller == larger);
    CPPUNIT_ASSERT(std::equal_to<T>()(smaller, larger));

    CPPUNIT_ASSERT(!(smaller != larger));
    CPPUNIT_ASSERT(!std::not_equal_to<T>()(smaller, larger));
  }
  else {
    CPPUNIT_ASSERT(smaller < larger);
    CPPUNIT_ASSERT(std::less<T>()(smaller, larger));

    CPPUNIT_ASSERT(smaller <= larger);
    CPPUNIT_ASSERT(std::less_equal<T>()(smaller, larger));

    CPPUNIT_ASSERT(larger > smaller);
    CPPUNIT_ASSERT(std::greater<T>()(larger, smaller));

    CPPUNIT_ASSERT(larger >= smaller);
    CPPUNIT_ASSERT(std::greater_equal<T>()(larger, smaller));

    CPPUNIT_ASSERT(!(smaller == larger));
    CPPUNIT_ASSERT(!std::equal_to<T>()(smaller, larger));

    CPPUNIT_ASSERT(smaller != larger);
    CPPUNIT_ASSERT(std::not_equal_to<T>()(smaller, larger));
  }

  // Copy-construction
  T copy(smaller);
  CPPUNIT_ASSERT(smaller == copy);
  if (equal) {
    CPPUNIT_ASSERT(larger == copy);
  }
  else {
    CPPUNIT_ASSERT(larger != copy);
  }

  // Assignment
  copy = larger;
  if (equal) {
    CPPUNIT_ASSERT(smaller == copy);
  }
  else {
    CPPUNIT_ASSERT(smaller != copy);
  }
  CPPUNIT_ASSERT(larger == copy);

  // Hash
  size_t s_hash = smaller.hash();
  size_t l_hash = larger.hash();
  if (equal) {
    CPPUNIT_ASSERT(s_hash == l_hash);
  }
  else {
    CPPUNIT_ASSERT(s_hash != l_hash);
  }

  std::hash<T> hasher;
  CPPUNIT_ASSERT(s_hash == hasher(smaller));

  // Swap
  T copy2 = smaller;
  if (equal) {
    CPPUNIT_ASSERT(copy == copy2);
  }
  else {
    CPPUNIT_ASSERT(copy2 < copy);
  }

  copy.swap(copy2);
  if (equal) {
    CPPUNIT_ASSERT(copy == copy2);
  }
  else {
    CPPUNIT_ASSERT(copy2 > copy);
    CPPUNIT_ASSERT(copy2 == larger);
  }

  std::swap(copy, copy2);
  if (equal) {
    CPPUNIT_ASSERT(copy == copy2);
  }
  else {
    CPPUNIT_ASSERT(copy2 < copy);
    CPPUNIT_ASSERT(copy2 == smaller);
  }

  // Return a copy of the larger value
  copy = larger;
  return copy;
}

#define PACKETEER_VALUES_TEST(smaller, larger, equal) \
  {                                                   \
    auto result = testValues(smaller, larger, equal); \
    CPPUNIT_ASSERT(result == larger);                 \
    if (equal) {                                      \
      CPPUNIT_ASSERT(result == smaller);              \
    }                                                 \
    else {                                            \
      CPPUNIT_ASSERT(result != smaller);              \
    }                                                 \
  }
