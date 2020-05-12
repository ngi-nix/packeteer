/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2014 Unwesen Ltd.
 * Copyright (c) 2015-2019 Jens Finkhaeuser.
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

#include "tasklet.h"

namespace packeteer::thread {

struct tasklet::tasklet_info
{
  tasklet *         m_tasklet;
  tasklet::function m_func;
  void *            m_thread_baton;

  tasklet_info(tasklet * tasklet, tasklet::function func, void * thread_baton)
    : m_tasklet(tasklet)
    , m_func(func)
    , m_thread_baton(thread_baton)
  {
  }
};


namespace {

static void tasklet_wrapper(void * arg)
{
  tasklet::tasklet_info * info = static_cast<tasklet::tasklet_info *>(arg);
  info->m_func(*(info->m_tasklet), info->m_thread_baton);
}

} // anonymous namespace


tasklet::tasklet(std::condition_variable_any * condition, std::recursive_mutex * mutex,
    tasklet::function const & func, void * baton /* = nullptr */, bool start_now /* = false */)
  : m_tasklet_info(new tasklet_info(this, func, baton))
  , m_condition(condition)
  , m_tasklet_mutex(mutex)
  , m_condition_owned(false)
  , m_mutex_owned(false)
{
  if (start_now) {
    start();
  }
}



tasklet::tasklet(tasklet::function const & func, void * baton /* = nullptr */,
    bool start_now /* = false */)
  : m_tasklet_info(new tasklet_info(this, func, baton))
  , m_condition(new std::condition_variable_any())
  , m_tasklet_mutex(new std::recursive_mutex())
  , m_condition_owned(true)
  , m_mutex_owned(true)
{
  if (start_now) {
    start();
  }
}



tasklet::~tasklet()
{
  stop();
  wait();
  delete m_tasklet_info;

  if (m_condition_owned) {
    delete m_condition;
  }
  if (m_mutex_owned) {
    delete m_tasklet_mutex;
  }
}



bool
tasklet::start()
{
  std::lock_guard<std::recursive_mutex> lock(*m_tasklet_mutex);

  if (m_thread.joinable()) {
    return false;
  }

  m_running = true;
  m_thread = std::thread(tasklet_wrapper, m_tasklet_info);
  return true;
}



bool
tasklet::stop()
{
  std::lock_guard<std::recursive_mutex> lock(*m_tasklet_mutex);

  if (!m_thread.joinable()) {
    return false;
  }

  m_running = false;

  if (m_condition_owned) {
    m_condition->notify_one();
  }
  else {
    m_condition->notify_all();
  }

  return true;
}



bool
tasklet::wait()
{
  if (m_thread.joinable()) {
    m_thread.join();
  }
  return true;
}



void
tasklet::wakeup()
{
  if (m_condition_owned) {
    m_condition->notify_one();
  }
  else {
    m_condition->notify_all();
  }
}



bool
tasklet::nanosleep(std::chrono::nanoseconds nsecs) const
{
  std::unique_lock<std::recursive_mutex> lock(*m_tasklet_mutex);

  if (!m_running) {
    return false;
  }

  // Negative numbers mean sleep infinitely.
  if (nsecs < std::chrono::nanoseconds(0)) {
    m_condition->wait(lock);
    return m_running;
  }

  // Sleep for a given time period only.
  m_condition->wait_for(lock, std::chrono::nanoseconds(nsecs));
  return m_running;
}


} // namespace packeteer::thread
