/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2017-2020 Jens Finkhaeuser.
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
#include <memory>

#include <liberate/cpp/hash.h>
#include <liberate/cpp/operators.h>


namespace packeteer {

/**
 * The handle class wraps I/O handles in a platform-independent fashion.
 **/
struct PACKETEER_API handle : public ::liberate::cpp::operators<handle>
{
#if defined(PACKETEER_WIN32)
  // The WIN32 definition of sys_handle_t is an opaque, reference counted
  // pointer.
  struct opaque_handle;
  using sys_handle_t = std::shared_ptr<opaque_handle>;
  static const sys_handle_t INVALID_SYS_HANDLE;

  // A few functions on the opaque structure are needed for the implementaiton
  // of the outer handle class.
private:
  static sys_handle_t sys_make_dummy(size_t const & value);
  static size_t sys_handle_hash(sys_handle_t const & handle);
  static bool sys_equal(sys_handle_t const & first, sys_handle_t const & second);
  static bool sys_less(sys_handle_t const & first, sys_handle_t const & second);
public:
#elif defined(PACKETEER_POSIX)
  using sys_handle_t = int;
  static const sys_handle_t INVALID_SYS_HANDLE{-1};
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

  // cppcheck-suppress noExplicitConstructor
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
    return static_cast<sys_handle_t>(value);
#elif defined(PACKETEER_WIN32)
    return sys_make_dummy(value);
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
 
#if defined(PACKETEER_POSIX)
   char const * p = reinterpret_cast<char const *>(&m_handle);
    size_t state = std::hash<char>()(p[0]);
    for (size_t i = 1 ; i < sizeof(sys_handle_t) ; ++i) {
      liberate::cpp::hash_combine(state, p[i]);
    }
    return state;
#elif defined(PACKETEER_WIN32)
    return sys_handle_hash(m_handle);
#endif // PACKETEER_POSIX
  }

  /**
   * Get underlying handle type
   **/
  sys_handle_t const & sys_handle() const
  {
    return m_handle;
  }

  sys_handle_t & sys_handle()
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
  friend struct ::liberate::cpp::operators<handle>;

  inline bool is_equal_to(handle const & other) const
  {
#if defined(PACKETEER_POSIX)
    return (0 == ::memcmp(&m_handle, &(other.m_handle), sizeof(sys_handle_t)));
#elif defined(PACKETEER_WIN32)
    return sys_equal(m_handle, other.m_handle);
#endif // PACKETEER_POSIX
  }

  inline bool is_less_than(handle const & other) const
  {
#if defined(PACKETEER_POSIX)
    return (::memcmp(&m_handle, &(other.m_handle), sizeof(sys_handle_t)) < 0);
#elif defined(PACKETEER_WIN32)
    return sys_less(m_handle, other.m_handle);
#endif // PACKETEER_POSIX
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
