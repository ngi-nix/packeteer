/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 * Copyright (c) 2012-2014 Unwesen Ltd.
 * Copyright (c) 2015-2020 Jens Finkhaeuser.
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
#ifndef PACKETEER_SCHEDULER_CALLBACK_H
#define PACKETEER_SCHEDULER_CALLBACK_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#include <typeinfo>
#include <functional>
#include <type_traits>

#include <packeteer/connector.h>

#include <packeteer/scheduler/events.h>
#include <packeteer/scheduler/types.h>

#include <packeteer/util/hash.h>
#include <packeteer/util/operators.h>

namespace packeteer {

namespace detail {

/*****************************************************************************
 * Internally used class.
 *
 * The callback_helper and its base allow callback to erase functor types,
 * but still be invoked correctly through the virtual invoke() function.
 **/
struct callback_helper_base
{
  virtual ~callback_helper_base() {}
  virtual error_t invoke(time_point const &, events_t const &, error_t,
      connector *, void *) = 0;
  virtual bool equal_to(callback_helper_base const *) const = 0;
  virtual bool less_than(callback_helper_base const *) const = 0;
  virtual size_t hash() const = 0;
  virtual callback_helper_base * clone() const = 0;
};


template <typename T>
struct callback_helper_member : public callback_helper_base
{
  typedef error_t (T::*member_function_type)(time_point const &, events_t,
      error_t, connector *, void *);


  T &                   m_object;
  member_function_type  m_function;

  inline callback_helper_member(T & obj, member_function_type func)
    : m_object(obj)
    , m_function(func)
  {
  }



  virtual error_t invoke(time_point const & now, events_t const & events,
      error_t error, connector * conn, void * baton)
  {
    // Here lies the trick of this construct: if a member function was given,
    // the implication is that it has an address (is not inline), so we invoke
    // it.
    // If one was given, we invoke operator(), which may or may not be declared
    // inline, as in e.g. the case of a lambda with capture.
    return (m_object.*m_function)(now, events, error, conn, baton);
    // FIXME cleanup
//    return m_object.operator()(now, events, error, conn, baton);
  }



  virtual bool equal_to(callback_helper_base const * other) const
  {
    if (typeid(*this) != typeid(*other)) {
      return false;
    }

    callback_helper_member<T> const * o
      = reinterpret_cast<callback_helper_member<T> const *>(other);

    return (&m_object == &(o->m_object) && m_function == o->m_function);
  }



  virtual bool less_than(callback_helper_base const * other) const
  {
    // This is a weird definition; we just need *some* ordering, and we know
    // they're unique.
    if (typeid(*this) != typeid(*other)) {
      return (&typeid(*this) < &typeid(*other));
    }

    callback_helper_member<T> const * o
      = reinterpret_cast<callback_helper_member<T> const *>(other);

    if (&m_object < &(o->m_object)) {
      return true;
    }
    return (m_function == o->m_function);
  }



  virtual size_t hash() const
  {
    return packeteer::util::multi_hash(
        reinterpret_cast<size_t>(&m_object),
        reinterpret_cast<size_t>(&typeid(T)));
  }



  virtual callback_helper_base * clone() const
  {
    return new callback_helper_member<T>(m_object, m_function);
  }
};


// FIXME document
template <typename T>
struct callback_helper_operator : public callback_helper_base
{
  T &                   m_object;

  inline callback_helper_operator(T & obj)
    : m_object(obj)
  {
  }



  virtual error_t invoke(time_point const & now, events_t const & events,
      error_t error, connector * conn, void * baton)
  {
    return m_object.operator()(now, events, error, conn, baton);
  }



  virtual bool equal_to(callback_helper_base const * other) const
  {
    if (typeid(*this) != typeid(*other)) {
      return false;
    }

    callback_helper_operator<T> const * o
      = reinterpret_cast<callback_helper_operator<T> const *>(other);

    return (&m_object == &(o->m_object));
  }



  virtual bool less_than(callback_helper_base const * other) const
  {
    // This is a weird definition; we just need *some* ordering, and we know
    // they're unique.
    if (typeid(*this) != typeid(*other)) {
      return (&typeid(*this) < &typeid(*other));
    }

    callback_helper_operator<T> const * o
      = reinterpret_cast<callback_helper_operator<T> const *>(other);

    return (&m_object < &(o->m_object));
  }



  virtual size_t hash() const
  {
    return packeteer::util::multi_hash(
        reinterpret_cast<size_t>(&m_object),
        reinterpret_cast<size_t>(&typeid(T)));
  }



  virtual callback_helper_base * clone() const
  {
    return new callback_helper_operator<T>(m_object);
  }
};

} // namespace detail




/*****************************************************************************
 * The callback class and associated free functions provide a much simplified
 * version of functionality from C++11's <functional> header - with the
 * important difference that it's possible to compare two callback objects.
 *
 * Part of the simplification means that callback objects don't hold any type
 * of function, only those conforming to the following prototype:
 *
 *  error_t <name>(time_point, events_t, error_t, connector *, void *)
 *
 * Much like std::function, however, callback classes can hold pointers to
 * free functions as well as pointers to object functions. Note that for the
 * latter, a pointer to the object is also required - and it's the
 * responsibility of the user that the object survives the callback.
 *
 * Usage:
 *  callback cb = &free_function;
 *  callback cb{&obj, &Object::function};
 *  callback cb{&obj}; // assumes operator() to be the function
 *  callback cb{obj, &Object::function};
 *  callback cb{obj}; // assumes operator() to be the function
 *
 * Note that callback does not take ownership of any functor object, whether
 * passed as a pointer or reference. That means the caller needs to ensure
 * the lifetime of the functor exceeds that of the callback instance.
 **/
class callback
  : public ::packeteer::util::operators<callback>
{
public:
  /*****************************************************************************
   * Types
   **/
  // Typedef for free functions.
  using free_function_type = std::add_pointer<
      error_t (time_point const &, events_t, error_t, connector *, void *)
  >::type;

  /*****************************************************************************
   * Implementation
   **/
  // Default constructor
  inline callback()
  {
  }


  // Construct from an object reference and member function
  template <
    typename T,
    typename std::enable_if_t<!std::is_pointer<T>::value>* = nullptr
  >
  inline callback(T & object,
      typename detail::callback_helper_member<T>::member_function_type func)
    : m_object_helper(new detail::callback_helper_member<T>{object, func})
  {
  }



  // Construct from an object pointer and member function
  template <
    typename T,
    typename std::enable_if_t<std::is_pointer<T>::value>* = nullptr
  >
  inline callback(T object_ptr,
      typename detail::callback_helper_member<
        typename std::remove_pointer<T>::type
      >::member_function_type func)
    : m_object_helper(new detail::callback_helper_member<
        typename std::remove_pointer<T>::type
      >{*object_ptr, func})
  {
  }



  // Construct from an object reference that is not a copy constructor. This
  // will be used by lambdas with capture.
  template <
    typename T,
    typename std::enable_if_t<!std::is_pointer<T>::value>* = nullptr,
    typename std::enable_if_t<!std::is_same<T, callback>::value>* = nullptr
  >
  inline callback(T & object)
    : m_object_helper(new detail::callback_helper_operator<T>{object})
  {
  }



  // Construct from an object pointer
  template <
    typename T,
    typename std::enable_if_t<std::is_pointer<T>::value>* = nullptr
  >
  inline callback(T object_ptr)
    : m_object_helper(new detail::callback_helper_operator<
        typename std::remove_pointer<T>::type
      >{*object_ptr})
  {
  }



  // Construct from a free function. This will be used by lambdas without
  // capture.
  inline callback(free_function_type free_func)
    : m_free_function(free_func)
  {
  }



  // Copy constructor
  inline callback(callback const & other)
    : m_free_function(other.m_free_function)
  {
    if (nullptr != other.m_object_helper) {
      // By cloning we can take ownership
      m_object_helper = other.m_object_helper->clone();
    }
  }



  // Move constructor
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
  operator()(time_point const & now, events_t events, error_t error,
      connector * conn, void * baton)
  {
    if (nullptr != m_free_function) {
      return (*m_free_function)(now, events, error, conn, baton);
    }
    else if (nullptr != m_object_helper) {
      return m_object_helper->invoke(now, events, error, conn, baton);
    }
    else {
      throw exception(ERR_EMPTY_CALLBACK);
    }
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
    return static_cast<size_t>(-1);
  }

private:
  friend struct ::packeteer::util::operators<callback>;

  inline bool is_equal_to(callback const & other) const
  {
    if (nullptr != m_free_function) {
      return m_free_function == other.m_free_function;
    }
    if (nullptr == other.m_object_helper
        || nullptr == m_object_helper)
    {
      return false;
    }
    return m_object_helper->equal_to(other.m_object_helper);
  }



  inline bool is_less_than(callback const & other) const
  {
    if (nullptr != m_free_function) {
      return (m_free_function < other.m_free_function);
    }

    if (nullptr == other.m_object_helper
        || nullptr == m_object_helper)
    {
      return m_object_helper < other.m_object_helper;
    }
    return m_object_helper->less_than(other.m_object_helper);
  }


  free_function_type              m_free_function = nullptr;
  detail::callback_helper_base *  m_object_helper = nullptr;
};


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


} // namespace std


#endif // guard
