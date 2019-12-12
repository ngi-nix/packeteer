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
#ifndef PACKETEER_HANDLE_H
#define PACKETEER_HANDLE_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#include <algorithm>
#include <utility>
#include <functional>
#include <cstring>
#include <iostream>

#include <packeteer/util/hash.h>
#include <packeteer/util/operators.h>


namespace packeteer {

/**
 * The handle class wraps I/O handles in a platform-independent fashion.
 **/
struct PACKETEER_API handle : public ::packeteer::util::operators<handle>
{
#if defined(PACKETEER_WIN32)
  struct PACKETEER_API sys_handle_t : public ::packeteer::util::operators<sys_handle_t>
  {
    HANDLE     handle = INVALID_HANDLE_VALUE;
    OVERLAPPED overlapped;

    inline bool is_equal_to(sys_handle_t const & other) const
    {
      return handle == other.handle;
    }

    inline bool is_less_than(sys_handle_t const & other) const
    {
      return handle < other.handle;
    }
  };
  const sys_handle_t INVALID_SYS_HANDLE{};
#elif defined(PACKETEER_POSIX)
  using sys_handle_t = int;
  const sys_handle_t INVALID_SYS_HANDLE{-1};
#else
#  error Handles are not supported on this platform!
#endif

  /**
   * Constructors and destructors
   **/
  handle()
    : m_handle{INVALID_SYS_HANDLE}
  {
  }

  handle(sys_handle_t const & orig)
    : m_handle{orig}
  {
  }

  handle(handle const & other) = default;
  handle(handle &&) = default;
  ~handle() = default;

  /**
   * Handles returned by this function behave like valid handles, but cannot
   * be used for I/O. Don't use this outside of code that requires dummy
   * handles.
   **/
  static handle make_dummy(size_t const & value)
  {
#if defined(PACKETEER_POSIX)
    return handle(static_cast<sys_handle_t>(value));
#else
    sys_handle_t h;
    h.handle = reinterpret_cast<HANDLE>(value);
    return handle(h);
#endif
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
    std::swap(m_handle, other.m_handle);
  }

  size_t hash() const
  {
    if (m_handle == INVALID_SYS_HANDLE) {
      return 0;
    }

    char const * p = reinterpret_cast<char const *>(&m_handle);
    size_t state = std::hash<char>()(p[0]);
    for (size_t i = 1 ; i < sizeof(sys_handle_t) ; ++i) {
      packeteer::util::hash_combine(state, p[i]);
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
    return (m_handle != INVALID_SYS_HANDLE);
  }


  /**
   * I/O
   **/
  friend std::ostream & operator<<(std::ostream & os, handle const & h);

private:
  friend struct ::packeteer::util::operators<handle>;

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


/**
 * Set/get the blocking mode of the file descriptor (on or off).
 */
PACKETEER_API error_t
set_blocking_mode(handle::sys_handle_t const & h, bool state);

PACKETEER_API error_t
get_blocking_mode(handle::sys_handle_t const & h, bool & state);

/**
 * Swappable
 **/
inline void
swap(::packeteer::handle & first, ::packeteer::handle & second)
{
  return first.swap(second);
}


} // namespace packeteer


/*******************************************************************************
 * std specializations for handle
 **/
namespace std {

template <> struct PACKETEER_API hash<::packeteer::handle>
{
  inline size_t operator()(::packeteer::handle const & conn)
  {
    return conn.hash();
  }
};


} // namespace std


#endif // guard
