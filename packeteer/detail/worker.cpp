/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 * Copyright (c) 2012-2014 Unwesen Ltd.
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
#include <packeteer/detail/worker.h>

#if defined(PACKETEER_HAVE_SYS_SELECT_H)
#  include <sys/select.h>
#elif defined(PACKETEER_HAVE_SYS_TYPES_H) && defined(PACKETEER_HAVE_UNISTD_H)
#  include <sys/types.h>
#  include <unistd.h>
#else
// FIXME #  error Cannot compile on this system.
#endif

#if defined(PACKETEER_HAVE_SYS_TIME_H)
#  include <sys/time.h>
#endif

#include <errno.h>

#include <packeteer/events.h>

namespace packeteer {
namespace detail {

worker::worker(std::condition_variable_any & condition, std::recursive_mutex & mutex,
    concurrent_queue<detail::callback_entry *> & work_queue)
  : packeteer::thread::tasklet(&condition, &mutex, packeteer::thread::binder(this, &worker::worker_loop))
  , m_alive(true)
  , m_work_queue(work_queue)
{
}



worker::~worker()
{
  stop();
}



void
worker::worker_loop(packeteer::thread::tasklet & tasklet, void * /* unused */)
{
  LOG("worker started");
  do {
    LOG("worker woke up");
    detail::callback_entry * entry = nullptr;
    while (m_work_queue.pop(entry)) {
      LOG("worker [" << std::hex << reinterpret_cast<uintptr_t>(this) << std::dec << "] picked up entry of type: " << entry->m_type);
      try {
        error_t err = execute_callback(entry);
        if (ERR_SUCCESS != err) {
          LOG("Error in callback: [" << error_name(err) << "] " << error_message(err));
        }

      } catch (exception const & ex) {
        ERR_LOG("Error in callback", ex);
      } catch (std::exception const & ex) {
        LOG("Error in callback: " << ex.what());
        delete entry;
      } catch (std::string const & str) {
        LOG("Error in callback: " << str);
        delete entry;
      } catch (...) {
        LOG("Error in callback.");
        delete entry;
      }
    }
    LOG("worker going to sleep");
  } while (tasklet.sleep());
  LOG("worker stopped");
}



error_t
worker::execute_callback(detail::callback_entry * entry)
{
  error_t err = ERR_SUCCESS;

  switch (entry->m_type) {
    case detail::CB_ENTRY_SCHEDULED:
      err = entry->m_callback(PEV_TIMEOUT, ERR_SUCCESS, handle(), nullptr);
      break;

    case detail::CB_ENTRY_USER:
      {
        auto user = reinterpret_cast<detail::user_callback_entry *>(entry);
        err = user->m_callback(user->m_events, ERR_SUCCESS, handle(), nullptr);
      }
      break;

    case detail::CB_ENTRY_IO:
      {
        auto io = reinterpret_cast<detail::io_callback_entry *>(entry);
        err = io->m_callback(io->m_events, ERR_SUCCESS, io->m_handle, nullptr);
      }
      break;

    default:
      // Unknown type. Signal an error on the callback.
      err = entry->m_callback(PEV_ERROR, ERR_UNEXPECTED, handle(), nullptr);
      break;
  }

  // Cleanup
  delete entry;
  return err;
}

}} // namespace packeteer::detail
