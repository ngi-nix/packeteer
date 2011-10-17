/**
 * This file is part of packetflinger.
 *
 * Author(s): Jens Finkhaeuser <unwesen@users.sourceforge.net>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **/
#ifndef PACKETFLINGER_CALLBACK_H
#define PACKETFLINGER_CALLBACK_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packetflinger/packetflinger.h>

#include <typeinfo>

#include <packetflinger/error.h>

namespace packetflinger {


namespace detail {

/*****************************************************************************
 * Internally used class.
 **/
struct callback_helper_base
{
  virtual error_t invoke(uint64_t events, error_t error, int fd, void * baton) = 0;
  virtual bool compare(callback_helper_base const * other) const = 0;
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
    : m_free_function(NULL)
    , m_object_helper(NULL)
  {
  }

  inline callback(free_function_type free_func)
    : m_free_function(free_func)
    , m_object_helper(NULL)
  {
  }

  inline callback(detail::callback_helper_base * helper)
    : m_free_function(NULL)
    , m_object_helper(helper) // take ownership
  {
  }

  inline callback(callback const & other)
    : m_free_function(other.m_free_function)
    , m_object_helper(NULL)
  {
    if (NULL != other.m_object_helper) {
      // By cloning we can take ownership
      m_object_helper = other.m_object_helper->clone();
    }
  }

  inline callback(callback && other)
    : m_free_function(other.m_free_function)
    , m_object_helper(other.m_object_helper)
   {
     other.m_free_function = NULL;
     other.m_object_helper = NULL;
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
    return (NULL == m_free_function && NULL == m_object_helper);
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
    m_object_helper = NULL;

    m_free_function = free_func;

    return *this;
  }


  inline callback & operator=(detail::callback_helper_base * helper)
  {
    delete m_object_helper;
    m_object_helper = helper; // take ownership

    m_free_function = NULL;

    return *this;
  }

  inline callback & operator=(callback const & other)
  {
    delete m_object_helper;
    m_object_helper = NULL;

    m_free_function = other.m_free_function;
    if (NULL != other.m_object_helper) {
      m_object_helper = other.m_object_helper->clone();
    }

    return *this;
  }

  inline callback & operator=(callback && other)
  {
    m_free_function = other.m_free_function;
    m_object_helper = other.m_object_helper;

    other.m_free_function = NULL;
    other.m_object_helper = NULL;

    return *this;
  }




  /**
   * Execute the bound function.
   **/
  inline
  error_t
  operator()(uint64_t events, error_t error, int fd, void * baton)
  {
    if (NULL != m_free_function) {
      return (*m_free_function)(events, error, fd, baton);
    }
    else if (NULL != m_object_helper) {
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
    if (NULL != m_free_function) {
      return m_free_function == other.m_free_function;
    }
    if (NULL == other.m_object_helper
        || NULL == m_object_helper)
    {
      return false;
    }
    return m_object_helper->compare(other.m_object_helper);
  }

  inline bool operator!=(callback const & other) const
  {
    return !operator==(other);
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
struct callback_helper : public callback_helper_base
{
  typedef error_t (T::*member_function_type)(uint64_t, error_t, int, void *);


  callback_helper(T * object, member_function_type function)
    : m_object(object)
    , m_function(function)
  {
  }



  virtual error_t invoke(uint64_t events, error_t error, int fd, void * baton)
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
detail::callback_helper<T> *
make_callback(T * object,
    typename detail::callback_helper<T>::member_function_type function)
{
  return new detail::callback_helper<T>(object, function);
}



template <typename T>
detail::callback_helper<T> *
make_callback(T * object)
{
  return new detail::callback_helper<T>(object, &T::operator());
}


} // namespace packetflinger

#endif // guard
