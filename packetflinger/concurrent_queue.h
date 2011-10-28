/**
 * This file is part of packetflinger.
 *
 * Author(s): Jens Finkhaeuser <unwesen@users.sourceforge.net>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
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
#ifndef PACKETFLINGER_CONCURRENT_QUEUE_H
#define PACKETFLINGER_CONCURRENT_QUEUE_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packetflinger/packetflinger.h>

#include <atomic>

#include <packetflinger/macros.h>

namespace packetflinger {

/*****************************************************************************
 * A simple concurrent queue implementation; lifted with small changes from
 * Herb Sutter's "Writing a Generalized Concurrent Queue"
 * http://drdobbs.com/high-performance-computing/211601363
 *
 * The queue implementation uses a producer spinlock and a consumer spinlock,
 * on the assumption that we'll have multiple producers and consumers.
 * Technically, we only have 1:N and N:1 situations, i.e. one producer and
 * multiple consumers or vice versa. In the interest of simplicity, we'll just
 * stick with the N:M implementation Sutter provides.
 *
 * The main change (other than some symbol name changes) is the addition of
 * the size() and empty() functions, which use the consumer lock and can
 * therefore contend with the consumers.
 *
 * Note that while this implementation uses STL-ish symbol names, it makes no
 * attempt at providing a full STL-like container.
 **/
template <typename valueT>
class concurrent_queue
{
public:
  /***************************************************************************
   * STL-ish typedefs
   **/
  typedef size_t size_type;
  typedef valueT value_type;



  /***************************************************************************
   * Implementation
   **/

  /**
   * Constructor/destructor
   **/
  inline concurrent_queue()
  {
    m_first = m_last = new node(nullptr);
    m_producer_lock = m_consumer_lock = false;
  }



  inline ~concurrent_queue()
  {
    while (nullptr != m_first) {
      node * tmp = m_first;
      m_first = tmp->m_next;
      delete tmp;
    }
  }



  /**
   * Add new values to the queue with push() and remove them with pop(). The
   * latter returns true if a value could be returned, false otherwise.
   *
   * Multiple producers using push() contend for a producer lock.
   * Multiple consumers using pop() contend for a consumer lock.
   *
   * Producers and consumers do not contend with each other, however.
   **/
  inline void push(valueT const & value)
  {
    node * tmp = new node(new valueT(value));

    while (m_producer_lock.exchange(true)) {}

    m_last->m_next = tmp;
    m_last = tmp;

    m_producer_lock = false;
  }



  template <typename iterT>
  inline void push_range(iterT const & begin, iterT const & end)
  {
    for (iterT iter = begin ; iter != end ; ++iter) {
      push(*iter);
    }
  }



  inline bool pop(valueT & result)
  {
    while (m_consumer_lock.exchange(true)) {}

    node * first = m_first;
    node * next = m_first->m_next;

    if (nullptr == next) {
      m_consumer_lock = false;
      return false;
    }

    valueT * val = next->m_value;
    next->m_value = nullptr;
    m_first = next;
    m_consumer_lock = false;

    result = *val;
    delete val;
    delete first;

    return true;
  }



  /**
   * STL-ish information functions on the state of the queue. Both take the
   * consumer's point of view and contend for the consumer lock with pop().
   *
   * Note that empty() is O(1), size() is O(N).
   *
   * It is *not* advisable to use empty() or size() for testing whether or not
   * pop() can be used.
   **/
  inline bool empty() const
  {
    while (m_consumer_lock.exchange(true)) {}

    bool ret = (nullptr == m_first->m_next);

    m_consumer_lock = false;

    return ret;
  }



  inline size_type size() const
  {
    while (m_consumer_lock.exchange(true)) {}

    size_type count = 0;
    node * cur = m_first->m_next;
    for ( ; nullptr != cur ; cur = cur->m_next, ++count) {}

    m_consumer_lock = false;

    return count;
  }


private:

  /**
   * Node for the internal linked list.
   **/
  struct node
  {
    node(valueT * value)
      : m_value(value)
      , m_next(nullptr)
    {
    }

    ~node()
    {
      m_next = nullptr;
      delete m_value;
    }

    PACKETFLINGER_CACHE_LINE_ALIGN(
      struct
      {
        valueT *            m_value;
        std::atomic<node *> m_next;
      };
    )
  };

  // By adding padding the size of a cache line, we ensure that the following
  // data members don't share a cache line with previously allocated data.
  PACKETFLINGER_CACHE_LINE_PAD;

  // The consumers contend for m_consumer_lock in order to use m_first.
  PACKETFLINGER_CACHE_LINE_ALIGN(node *                     m_first);
  PACKETFLINGER_CACHE_LINE_ALIGN(mutable std::atomic<bool>  m_consumer_lock);

  // The producers contend for m_producer_lock in order to use m_last.
  PACKETFLINGER_CACHE_LINE_ALIGN(node *                     m_last);
  PACKETFLINGER_CACHE_LINE_ALIGN(mutable std::atomic<bool>  m_producer_lock);
};

} // namespace packetflinger

#endif // guard
