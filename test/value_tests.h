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
#ifndef PACKETEER_TEST_VALUE_TESTS_H
#define PACKETEER_TEST_VALUE_TESTS_H

#include <gtest/gtest.h>

#include <functional>

/**
 * Simple test case for value types where we expect both values to be equal.
 */
template <typename T>
inline void
test_equality(T const & first, T const & second)
{
  // The obvious one - must be equal.
  ASSERT_EQ(first, second);
  ASSERT_TRUE(std::equal_to<T>()(first, second));

  // Not-equal comparisons must be false
  ASSERT_FALSE(first != second);
  ASSERT_FALSE(std::not_equal_to<T>()(first, second));

  // Less-equal and greater-equal must work
  ASSERT_TRUE(first <= second);
  ASSERT_TRUE(std::less_equal<T>()(first, second));

  ASSERT_TRUE(first >= second);
  ASSERT_TRUE(std::greater_equal<T>()(first, second));

  // However, without the greater part, it must not.
  ASSERT_FALSE(first < second);
  ASSERT_FALSE(std::less<T>()(first, second));

  ASSERT_FALSE(first > second);
  ASSERT_FALSE(std::greater<T>()(first, second));
}

/**
 * Similar to test_equality, but assumes non-equality, specifically the first
 * element must evaluate as less than the second.
 */
template <typename T>
inline void
test_less_than(T const & lesser, T const & greater)
{
  // The obvious one - must *not* be equal.
  ASSERT_NE(lesser, greater);
  ASSERT_FALSE(std::equal_to<T>()(lesser, greater));

  // Not-equal comparisons must be true
  ASSERT_TRUE(lesser != greater);
  ASSERT_TRUE(std::not_equal_to<T>()(lesser, greater));

  // Less-equal and less-than must be true
  ASSERT_TRUE(lesser <= greater);
  ASSERT_TRUE(std::less_equal<T>()(lesser, greater));

  ASSERT_TRUE(lesser < greater);
  ASSERT_TRUE(std::less<T>()(lesser, greater));

  // Greater-equal and greater-than must be false.
  ASSERT_FALSE(lesser > greater);
  ASSERT_FALSE(std::greater<T>()(lesser, greater));

  ASSERT_FALSE(lesser >= greater);
  ASSERT_FALSE(std::greater_equal<T>()(lesser, greater));
}


/**
 * Test copy construction
 */
template <typename T>
inline void
test_copy_construction(T const & value)
{
  T copy{value};
  ASSERT_EQ(value, copy);
}


/**
 * Test assignment (and copy construction)
 */
template <typename T>
inline void
test_assignment(T const & value)
{
  T copy = T{value};
  ASSERT_EQ(value, copy);
}


/**
 * Test hashing (and copy construction). Parameters are expected to not
 * hash equally, but copies are hashed equally.
 */
template <typename T>
inline void
test_hashing_equality(T const & first, T const & second)
{
  EXPECT_EQ(first, second);

  auto hash1 = std::hash<T>{}(first);
  auto hash2 = std::hash<T>{}(second);
  ASSERT_EQ(hash1, hash2);

  T copy1{first};
  ASSERT_EQ(hash1, std::hash<T>{}(copy1));

  T copy2{second};
  ASSERT_EQ(hash2, std::hash<T>{}(copy2));
}


template <typename T>
inline void
test_hashing_inequality(T const & first, T const & second)
{
  EXPECT_NE(first, second);

  auto hash1 = std::hash<T>{}(first);
  auto hash2 = std::hash<T>{}(second);
  ASSERT_NE(hash1, hash2);

  T copy1{first};
  ASSERT_EQ(hash1, std::hash<T>{}(copy1));

  T copy2{second};
  ASSERT_EQ(hash2, std::hash<T>{}(copy2));
}



/**
 * Test swapping (and copy construction). Parameters are expected to not
 * be equal, so that creating copies and swapping their values will
 * checkable.
 */
template <typename T>
inline void
test_swapping(T const & first, T const & second)
{
  EXPECT_NE(first, second);

  T copy1{first};
  T copy2{second};

  std::swap(copy1, copy2);

  ASSERT_EQ(copy1, second);
  ASSERT_EQ(copy2, first);
}


/**
 * Test incrementing (and copying). The incremented value must not be equal to
 * the original value.
 */
template <typename T>
inline void
test_incrementing(T const & value)
{
  T copy{value};

  ASSERT_EQ(copy, value);
  ++copy;

  ASSERT_NE(copy, value);
  ASSERT_GT(copy, value);
}


#endif // guard
