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
#include <atomic>

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
  virtual size_t hash() const = 0;
  virtual callback_helper_base * clone() const = 0;
};



/**
 * The holder class is a not-so-smart-pointer light, or whatever you would
 * like to call it. It combines an object pointer with a reference count,
 * and a flag whether or not the object is owned.
 *
 * It's used in callback_helper_base implementations to model both ownership
 * and non-ownership of the functor they're proxying.
 **/
template <typename T>
struct holder
{
  T *               m_object = nullptr;
  bool const        m_owned = false;
  std::atomic<int>  m_refcount = 1;

  inline holder(T * obj)
    : m_object(obj)
  {
  }

  inline holder(T const & obj)
    : m_object(new T{obj})
    , m_owned(true)
  {
  }

  inline holder * increment()
  {
    ++m_refcount;
    return this;
  }

  inline bool delete_if_last_reference()
  {
    if (--m_refcount <= 0) {
      if (m_owned) {
        delete m_object;
      }
      m_object = nullptr;
      return true;
    }
    return false;
  }
};



/**
 * A proxy for an object and member function.
 *
 * If it's constructed from a pointer and member function pointer, the
 * object is not considered owned. Otherwise, a copy is made and the copy
 * is owned - but only one copy is made thanks to the holder construct.
 **/
template <typename T>
struct callback_helper_member : public callback_helper_base
{
  typedef error_t (T::*member_function_type)(time_point const &, events_t,
      error_t, connector *, void *);

  holder<T> *           m_holder;
  member_function_type  m_function;
  size_t                m_hash = 0;

  inline callback_helper_member(T * obj, member_function_type func)
    : m_holder(new holder<T>{obj})
    , m_function(func)
    , m_hash(hash_of(obj))
  {
  }



  inline callback_helper_member(T const & obj, member_function_type func)
    : m_holder(new holder<T>{obj})
    , m_function(func)
    , m_hash(hash_of(&obj))
  {
  }



  ~callback_helper_member()
  {
    if (m_holder->delete_if_last_reference()) {
      delete m_holder;
    }
    m_holder = nullptr;
  }



  virtual error_t invoke(time_point const & now, events_t const & events,
      error_t error, connector * conn, void * baton)
  {
    // Here lies the trick of this construct: if a member function was given,
    // the implication is that it has an address (is not inline), so we invoke
    // it.
    // If one was given, we invoke operator(), which may or may not be declared
    // inline, as in e.g. the case of a lambda with capture.
    return (m_holder->m_object->*m_function)(now, events, error, conn, baton);
  }



  virtual size_t hash() const
  {
    return m_hash;
  }



  virtual callback_helper_base * clone() const
  {
    auto res = new callback_helper_member<T>();
    res->m_holder = m_holder->increment();
    res->m_function = m_function;
    res->m_hash = m_hash;
    return res;
  }

private:
  callback_helper_member()
  {
  }


  inline size_t hash_of(T const * obj)
  {
    return packeteer::util::multi_hash(
        reinterpret_cast<size_t>(obj),
        reinterpret_cast<size_t>(&typeid(member_function_type)),
        reinterpret_cast<size_t>(&typeid(T)));
  }
};


/**
 * A proxy for a functor object.
 *
 * Ownership works the same as in callback_helper_member, but no member
 * function is used. Instead, calls are redirected to operator() - if
 * that is inline because it's a lambda with capture, that works.
 **/
template <typename T>
struct callback_helper_operator : public callback_helper_base
{
  holder<T> * m_holder;
  size_t      m_hash;

  inline callback_helper_operator(T * obj)
    : m_holder(new holder<T>{obj})
    , m_hash(hash_of(obj))
  {
  }



  inline callback_helper_operator(T const & obj)
    : m_holder(new holder<T>{obj})
    , m_hash(hash_of(&obj))
  {
  }



  ~callback_helper_operator()
  {
    if (m_holder->delete_if_last_reference()) {
      delete m_holder;
    }
    m_holder = nullptr;
  }



  virtual error_t invoke(time_point const & now, events_t const & events,
      error_t error, connector * conn, void * baton)
  {
    return m_holder->m_object->operator()(now, events, error, conn, baton);
  }



  virtual size_t hash() const
  {
    return m_hash;
  }



  virtual callback_helper_base * clone() const
  {
    auto res = new callback_helper_operator<T>();
    res->m_holder = m_holder->increment();
    res->m_hash = m_hash;
    return res;
  }

private:
  callback_helper_operator()
  {
  }


  inline size_t hash_of(T const * obj)
  {
    return packeteer::util::multi_hash(
        reinterpret_cast<size_t>(obj),
        reinterpret_cast<size_t>(&typeid(T)));
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
 *
 *  // By pointer
 *  callback cb{&obj, &Object::function};
 *  callback cb{&obj}; // assumes operator() to be the function
 *
 *  // Copies obj - Object must be copy-constructible
 *  callback cb{obj, &Object::function};
 *  callback cb{obj}; // assumes operator() to be the function
 *
 * If you're constructing a callback via an object pointer, then the callback
 * does not take ownership. You have to ensure yourself that the pointed-to
 * object's lifetime is longer than that of the callback.
 *
 * If you're constructing a callback via an object (const) reference, a copy
 * of that object is made and managed by callback. You should not use this for
 * objects that are heavy to copy - on the other hand, you do not need to
 * ensure anything about the lifetime of the object being copied.
 *
 * Lambdas without captures convert to free function pointers, so are very
 * lightweight callbacks without any internally managed copies of anything.
 * Lambdas with capture convert to objects, and would typically be copied,
 * with the copy managed by callback.
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
  inline callback(T const & object,
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
      >{object_ptr, func})
  {
  }



  // Construct from an object reference that is not a copy constructor. This
  // will be used by lambdas with capture or lambdas returned from functions.
  template <
    typename T,
    typename std::enable_if_t<!std::is_pointer<T>::value>* = nullptr,
    typename std::enable_if_t<!std::is_same<T, callback>::value>* = nullptr,
    typename std::enable_if_t<!std::is_convertible<T, free_function_type>::value>* = nullptr
  >
  inline callback(T const & object)
    : m_object_helper(new detail::callback_helper_operator<T>{object})
  {
  }



  // Construct from an object pointer
  template <
    typename T,
    typename std::enable_if_t<std::is_pointer<T>::value>* = nullptr,
    typename std::enable_if_t<!std::is_convertible<T, free_function_type>::value>* = nullptr
  >
  inline callback(T object_ptr)
    : m_object_helper(new detail::callback_helper_operator<
        typename std::remove_pointer<T>::type
      >{object_ptr})
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
    return m_object_helper->hash() == other.m_object_helper->hash();
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
    return m_object_helper->hash() < other.m_object_helper->hash();
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
