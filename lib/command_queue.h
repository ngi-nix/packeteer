/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2020 Jens Finkhaeuser.
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
#ifndef PACKETEER_COMMAND_QUEUE_H
#define PACKETEER_COMMAND_QUEUE_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <build-config.h>

#include <tuple>

#include <packeteer/connector.h>

#include <liberate/concurrency/concurrent_queue.h>

#include "interrupt.h"

namespace packeteer::detail {

/**
 * The command_queue type takes a tuple of a command and its payload into
 * a concurrent queue.
 *
 * Note that arguments are copied into the queue, and copied out of the queue,
 * as you would expect with standard containers.
 */
template <
  typename commandT,
  typename... argsT
>
class command_queue
{
public:
  using value_type = std::tuple<commandT, argsT...>;
  using key_type = commandT;

  inline void enqueue(commandT const & command, argsT &&... args)
  {
    m_queue.push(std::forward_as_tuple(command, args...));
  }

  inline void enqueue(commandT const & command, argsT const &... args)
  {
    m_queue.push(std::forward_as_tuple(command, args...));
  }

  inline bool dequeue(commandT & command, argsT &... args)
  {
    value_type entry;
    auto ret = m_queue.pop(entry);
    if (!ret) return false;
    std::tie(command, args...) = entry;
    return true;
  }

private:
  using queue_t = liberate::concurrency::concurrent_queue<value_type>;

  queue_t m_queue;
};



/**
 * The command_queue_with_signal is a simple extension to the above
 * command_queue: it also holds a connector, and contains a commit()
 * method that interrupts the connector as a signal. This way, multiple
 * commands can be enqueued before signalling a different thread to pick
 * them up.
 *
 * Note that the signal state and the queue size are entirely indepdendent
 * of each other. It is possible to commit an empty queue, and keep the queue
 * full after clearing the interrupt. However, bundling both parameters makes
 * it possible to pass the queue and its signalling mechanism as a single
 * parameter.
 */
template <
  typename commandT,
  typename... argsT
>
class command_queue_with_signal : public command_queue<commandT, argsT...>
{
public:
  inline command_queue_with_signal(connector & signal)
    : m_connector(signal)
  {
  }

  inline connector & signal()
  {
    return m_connector;
  }

  inline void commit()
  {
    set_interrupt(m_connector);
  }

  inline bool clear()
  {
    return clear_interrupt(m_connector);
  }

private:
  connector & m_connector;
};




} // namespace packeteer::detail

#endif // guard
