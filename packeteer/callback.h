/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@unwesen.co.uk>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 * Copyright (c) 2012-2014 Unwesen Ltd.
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
#ifndef PACKETEER_CALLBACK_H
#define PACKETEER_CALLBACK_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer/packeteer.h>

#include <typeinfo>
#include <functional>

#include <meta/hash.h>

#include <packeteer/error.h>
#include <packeteer/events.h>

namespace packeteer {


namespace detail {

/*****************************************************************************
 * Internally used class.
 **/
struct callback_helper_base
{
  virtual ~callback_helper_base() {}
  virtual error_t invoke(events_t const & events, error_t error, int fd, void * baton) = 0;
  virtual bool compare(callback_helper_base const * other) const = 0;
  virtual size_t hash() const = 0;
  virtual callback_helper_base * clone() const = 0;
};


} // namespace detail;



/*****************************************************************************
 * The callback class and associated free functions provide a much simplified
 * version of functionality from C++11's <functional> header - with the
 * important difference that it's possible to compare two callback objects.
 *
 * Part of the simplification means that callback objects don't hold any type
 * of function, only those conforming to the following prototype:
 *
 *  error_t <name>(uint64_t events, error_t error, int fd, void * baton)
 *
 * Much like std::function, however, callback classes can hold pointers to
 * free functions as well as pointers to object functions. Note that for the
 * latter, a pointer to the object is also required - and it's the
 * responsibility of the user that the object survives the callback.
 *
 * Usage:
 *  callback cb = &free_function;
 *  callback cb = make_callback(&obj, &Object::function);
 *  callback cb = make_callback(&obj); // assumes operator() to be the function
 **/
class callback
{
public:
  /*****************************************************************************
   * Types
   **/
  // Typedef for free functions.
  typedef error_t (*free_function_type)(uint64_t, error_t, int, void *);


  /*****************************************************************************
   * Implementation
   **/
  inline callback()
    : m_free_function(nullptr)
    , m_object_helper(nullptr)
  {
  }

  inline callback(free_function_type free_func)
    : m_free_function(free_func)
    , m_object_helper(nullptr)
  {
  }

  inline callback(detail::callback_helper_base * helper)
    : m_free_function(nullptr)
    , m_object_helper(helper) // take ownership
  {
  }

  inline callback(callback const & other)
    : m_free_function(other.m_free_function)
    , m_object_helper(nullptr)
  {
    if (nullptr != other.m_object_helper) {
      // By cloning we can take ownership
      m_object_helper = other.m_object_helper->clone();
    }
  }

  inline callback(callback && other)
    : m_free_function(other.m_free_function)
    , m_object_helper(other.m_object_helper)
   {
     other.m_free_function = nullptr;
     other.m_object_helper = nullptr;
   }



  inline ~callback()
  {
    delete m_object_helper;
  }



  /**
   * Does callback hold a function or not?
   **/
  inline bool empty() const
  {
    return (nullptr == m_free_function && nullptr == m_object_helper);
  }

  inline operator bool() const
  {
    return !empty();
  }




  /**
   * Assignment
   **/
  inline callback & operator=(free_function_type free_func)
  {
    delete m_object_helper;
    m_object_helper = nullptr;

    m_free_function = free_func;

    return *this;
  }


  inline callback & operator=(detail::callback_helper_base * helper)
  {
    delete m_object_helper;
    m_object_helper = helper; // take ownership

    m_free_function = nullptr;

    return *this;
  }

  inline callback & operator=(callback const & other)
  {
    delete m_object_helper;
    m_object_helper = nullptr;

    m_free_function = other.m_free_function;
    if (nullptr != other.m_object_helper) {
      m_object_helper = other.m_object_helper->clone();
    }

    return *this;
  }

  inline callback & operator=(callback && other)
  {
    m_free_function = other.m_free_function;
    m_object_helper = other.m_object_helper;

    other.m_free_function = nullptr;
    other.m_object_helper = nullptr;

    return *this;
  }




  /**
   * Execute the bound function.
   **/
  inline
  error_t
  operator()(uint64_t events, error_t error, int fd, void * baton)
  {
    if (nullptr != m_free_function) {
      return (*m_free_function)(events, error, fd, baton);
    }
    else if (nullptr != m_object_helper) {
      return m_object_helper->invoke(events, error, fd, baton);
    }
    else {
      throw exception(ERR_EMPTY_CALLBACK);
    }
  }



  /**
   * Equality-compare two callback objects.
   **/
  inline bool operator==(callback const & other) const
  {
    if (nullptr != m_free_function) {
      return m_free_function == other.m_free_function;
    }
    if (nullptr == other.m_object_helper
        || nullptr == m_object_helper)
    {
      return false;
    }
    return m_object_helper->compare(other.m_object_helper);
  }

  inline bool operator!=(callback const & other) const
  {
    return !operator==(other);
  }



  /**
   * Hash values
   **/
  inline size_t hash() const
  {
    if (nullptr != m_free_function) {
      return std::hash<size_t>()(reinterpret_cast<size_t>(m_free_function));
    }
    if (nullptr != m_object_helper) {
      return m_object_helper->hash();
    }
    return -1;
  }

private:
  free_function_type              m_free_function;
  detail::callback_helper_base *  m_object_helper;
};


/*******************************************************************************
 * The function_helper class template holds object and member function pointers.
 **/
namespace detail {

template <typename T>
void
hash_helper()
{
}


template <typename T>
struct callback_helper : public callback_helper_base
{
  typedef error_t (T::*member_function_type)(uint64_t, error_t, int, void *);


  callback_helper(T * object, member_function_type function)
    : m_object(object)
    , m_function(function)
  {
  }



  virtual error_t invoke(events_t const & events, error_t error, int fd, void * baton)
  {
    return (m_object->*m_function)(events, error, fd, baton);
  }



  virtual bool compare(callback_helper_base const * other) const
  {
    if (typeid(*this) != typeid(*other)) {
      return false;
    }

    callback_helper<T> const * o
      = reinterpret_cast<callback_helper<T> const *>(other);

    return (m_object == o->m_object && m_function == o->m_function);
  }



  virtual size_t hash() const
  {
    return meta::hash::multi_hash(
        reinterpret_cast<size_t>(m_object),
        reinterpret_cast<size_t>(&hash_helper<T>));
  }



  virtual callback_helper_base * clone() const
  {
    return new callback_helper<T>(m_object, m_function);
  }

private:
  T *                   m_object;
  member_function_type  m_function;
};

} // namespace detail


/*******************************************************************************
 * Free functions for binding object methods to a callback object. The first
 * form takes a object pointer and member function pointer as arguments, the
 * second only an object pointer - the member function is then assumed to be
 * operator().
 **/
template <typename T>
inline detail::callback_helper<T> *
make_callback(T * object,
    typename detail::callback_helper<T>::member_function_type function)
{
  return new detail::callback_helper<T>(object, function);
}



template <typename T>
inline detail::callback_helper<T> *
make_callback(T * object)
{
  return new detail::callback_helper<T>(object, &T::operator());
}


} // namespace packeteer


/*******************************************************************************
 * std namespace specializations
 **/
namespace std {

template <> struct hash<packeteer::callback>
{
  size_t operator()(packeteer::callback const & x) const
  {
    return x.hash();
  }
};

template <> struct equal_to<packeteer::callback>
{
  bool operator()(packeteer::callback const & x, packeteer::callback const & y) const
  {
    return x.operator==(y);
  }
};

} // namespace std

#endif // guard
