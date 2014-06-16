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
#include <packeteer/detail/worker.h>

#if defined(PACKETEER_HAVE_SYS_SELECT_H)
#  include <sys/select.h>
#elif defined(PACKETEER_HAVE_SYS_TIME_H) && defined(PACKETEER_HAVE_SYS_TYPES_H) && defined(PACKETEER_HAVE_UNISTD_H)
#  include <sys/types.h>
#  include <unistd.h>
#else
#  error Cannot compile on this system.
#endif

#include <sys/time.h>
#include <errno.h>

#include <packeteer/events.h>

namespace packeteer {
namespace detail {

worker::worker(twine::condition & condition, twine::recursive_mutex & mutex,
    concurrent_queue<detail::callback_entry *> & work_queue)
  : twine::tasklet(&condition, &mutex, twine::tasklet::binder<worker, &worker::worker_loop>::function, this)
  , m_alive(true)
  , m_work_queue(work_queue)
{
}



worker::~worker()
{
  stop();
}



//void
//worker::interrupt()
//{
//  char buf[1] = { '\0' };
//  m_pipe.write(buf, sizeof(buf));
//}
//
//
//
//void
//worker::sleep()
//{
//  // Similar to duration::sleep, but sleeps indefinitely, and waits for an
//  // interruption on the pipe's read file descriptor.
//  ::fd_set read_fds;
//  FD_ZERO(&read_fds);
//
//  ::fd_set err_fds;
//  FD_ZERO(&err_fds);
//
//  int read_fd = m_pipe.get_read_fd();
//  FD_SET(read_fd, &read_fds);
//  FD_SET(read_fd, &err_fds);
//
//  do {
//    int ret = ::select(read_fd + 1, &read_fds, nullptr, &err_fds, nullptr);
//
//    if (0 == ret) {
//      // Hmm, shouldn't happen. Still, it's not an error, so continue.
//      continue;
//    }
//
//    if (-1 == ret) {
//      if (EINTR == errno) {
//        continue;
//      }
//      else {
//        // TODO throw
//        LOG("select errno: " << errno);
//        break;
//      }
//    }
//
//    LOG("read from FDs: " << ret);
//    if (FD_ISSET(read_fd, &read_fds)) {
//      LOG("sleep interrupted");
//      detail::clear_interrupt(m_pipe);
//      break;
//    }
//    else if (FD_ISSET(read_fd, &err_fds)) {
//      LOG("error on pipe FD");
//      break;
//    }
//    else {
//      // TODO throw ?
//      LOG("pipe FD not in FD set, yet it was the only one added");
//      break;
//    }
//  } while (true);
//
//  LOG("returning from sleep");
//}



void
worker::worker_loop(twine::tasklet & tasklet, void * /* unused */)
{
  LOG("worker started");
  do {
    LOG("worker woke up");
    detail::callback_entry * entry = nullptr;
    while (m_work_queue.pop(entry)) {
      LOG("worker picked up entry of type: " << entry->m_type);
      execute_callback(entry);
      // FIXME should be a delete here, but memory management is still shot.
      // delete entry;
    }
    LOG("worker going to sleep");
  } while (tasklet.sleep());
  LOG("worker stopped");
}



void
worker::execute_callback(detail::callback_entry * entry)
{
  error_t err = ERR_SUCCESS;

  switch (entry->m_type) {
    case detail::CB_ENTRY_SCHEDULED:
      err = entry->m_callback(EV_TIMEOUT, ERR_SUCCESS, -1, nullptr);
      break;

    case detail::CB_ENTRY_USER:
      {
        auto user = reinterpret_cast<detail::user_callback_entry *>(entry);
        err = user->m_callback(user->m_events, ERR_SUCCESS, -1, nullptr);
      }
      break;

    case detail::CB_ENTRY_IO:
      // TODO
      // FIXME break;

    default:
      // Unknown type. Signal an error on the callback.
      err = entry->m_callback(EV_ERROR, ERR_UNEXPECTED, -1, nullptr);
      break;
  }

  delete entry;
}

}} // namespace packeteer::detail