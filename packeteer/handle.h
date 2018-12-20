/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2017 Jens Finkhaeuser.
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
#ifndef PACKETEER_HANDLE_H
#define PACKETEER_HANDLE_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer/packeteer.h>

#include <algorithm>
#include <utility>
#include <functional>
#include <cstring>
#include <iostream>

#include <meta/hash.h>

#include <packeteer/util/operators.h>


namespace packeteer {

/**
 * The handle class wraps I/O handles in a platform-independent fashion.
 **/
struct handle : public ::packeteer::util::operators<handle>
{
#if defined(PACKETEER_WIN32)
  typedef HANDLE  sys_handle_t;
#  define PACKETEER_INVALID_HANDLE_VALUE INVALID_HANDLE_VALUE
#elif defined(PACKETEER_POSIX)
  typedef int     sys_handle_t;
#  define PACKETEER_INVALID_HANDLE_VALUE -1
#else
#  error Handles are not supported on this platform!
#endif

  /**
   * Constructors and destructors
   **/
  handle()
    : m_handle(PACKETEER_INVALID_HANDLE_VALUE)
  {
  }

  handle(sys_handle_t const & orig)
    : m_handle(orig)
  {
  }

  handle(handle const & other)
    : m_handle(other.m_handle)
  {
  }

  handle(handle &&) = default;

  ~handle()
  {
  }

  /**
   * Operators
   **/
  handle & operator=(handle const & other)
  {
    m_handle = other.m_handle;
    return *this;
  }

  /**
   * Utility
   **/
  void swap(handle & other)
  {
    sys_handle_t tmp = m_handle;
    m_handle = other.m_handle;
    other.m_handle = tmp;
  }

  size_t hash() const
  {
    char const * p = reinterpret_cast<char const *>(&m_handle);
    size_t state = std::hash<char>()(p[0]);
    for (size_t i = 1 ; i < sizeof(sys_handle_t) ; ++i) {
      state = meta::hash::hash_combine(state, p[i]);
    }
    return state;
  }

  /**
   * Get underlying handle type
   **/
  sys_handle_t sys_handle() const
  {
    return m_handle;
  }

  bool valid() const
  {
    return (m_handle != PACKETEER_INVALID_HANDLE_VALUE);
  }


  /**
   * I/O
   **/
  friend std::ostream & operator<<(std::ostream & os, handle const & h);

private:
  friend class ::packeteer::util::operators<handle>;

  inline bool is_equal_to(handle const & other) const
  {
    return (0 == ::memcmp(&m_handle, &(other.m_handle), sizeof(sys_handle_t)));
  }

  inline bool is_less_than(handle const & other) const
  {
    return (::memcmp(&m_handle, &(other.m_handle), sizeof(sys_handle_t)) < 0);
  }

  sys_handle_t  m_handle;
};


/**
 * I/O
 **/
inline std::ostream &
operator<<(std::ostream & os, handle const & h)
{
  os << h.hash();
  return os;
}


} // namespace packeteer

/**
 * std specializations for handle
 **/
namespace std {

template <> struct hash<::packeteer::handle>
{
  inline size_t operator()(::packeteer::handle const & conn)
  {
    return conn.hash();
  }
};


template <>
inline void
swap<::packeteer::handle>(::packeteer::handle & first, ::packeteer::handle & second)
{
  return first.swap(second);
}

} // namespace std



#endif // guard
