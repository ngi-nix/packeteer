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
#include <packetflinger/scheduler.h>

#include <packetflinger/detail/scheduler_impl.h>
#include <packetflinger/detail/worker.h>

namespace pdu = packetflinger::duration;
namespace pdt = packetflinger::detail;

namespace packetflinger {

/*****************************************************************************
 * Free detail functions
 **/
namespace detail {

void interrupt(pipe & pipe)
{
  char buf[1] = { '\0' };
  pipe.write(buf, sizeof(buf));
}



void clear_interrupt(pipe & pipe)
{
  char buf[1] = { '\0' };
  size_t amount = 0;
  pipe.read(buf, sizeof(buf), amount);
}

} // namespace detail






/*****************************************************************************
 * class scheduler::scheduler_impl
 **/
scheduler::scheduler_impl::scheduler_impl(size_t num_worker_threads)
  : m_num_worker_threads(num_worker_threads)
  , m_workers()
  , m_worker_pipe()
  , m_main_loop_continue(true)
  , m_main_loop_thread()
  , m_main_loop_pipe()
  , m_in_queue()
  , m_out_queue()
  , m_scheduled_callbacks()
{
  init_impl();

  start_main_loop();
  start_workers(m_num_worker_threads);
}



scheduler::scheduler_impl::~scheduler_impl()
{
  stop_workers(m_num_worker_threads);
  stop_main_loop();

  deinit_impl();
}



void
scheduler::scheduler_impl::enqueue(pdt::callback_entry * entry)
{
  m_in_queue.push(entry);
}



void
scheduler::scheduler_impl::start_main_loop()
{
  m_main_loop_continue = true;

  register_fd(m_main_loop_pipe.get_read_fd(),
      EV_IO_READ | EV_IO_ERROR | EV_IO_CLOSE);

  m_main_loop_thread = boost::thread(
      std::bind(&scheduler_impl::main_scheduler_loop, this));
}



void
scheduler::scheduler_impl::stop_main_loop()
{
  m_main_loop_continue = false;

  detail::interrupt(m_main_loop_pipe);
  m_main_loop_thread.join();

  unregister_fd(m_main_loop_pipe.get_read_fd(),
      EV_IO_READ | EV_IO_ERROR | EV_IO_CLOSE);
}



void
scheduler::scheduler_impl::start_workers(size_t num_workers)
{
  for (size_t i = m_workers.size() ; i < num_workers ; ++i) {
    m_workers.push_back(new pdt::worker(m_worker_pipe, m_out_queue));
  }
}



void
scheduler::scheduler_impl::stop_workers(size_t num_workers)
{
  size_t current = m_workers.size();
  for (size_t i = current - 1; i >= num_workers ; --i) {
    pdt::worker * worker = m_workers[i];
    m_workers.pop_back();
    delete worker; // joins
  }
}



void
scheduler::scheduler_impl::process_in_queue()
{
  pdt::callback_entry * entry = nullptr;
  while (m_in_queue.pop(entry)) {
    if (nullptr == entry) {
      continue;
    }


    switch (entry->m_type) {
      case pdt::CB_ENTRY_SCHEDULED:
        {
          pdt::scheduled_callback_entry * scheduled
            = reinterpret_cast<pdt::scheduled_callback_entry *>(entry);

          if (scheduled->m_add) {
            // When adding, we simply add scheduled entries. It's entirely
            // possible that the same (callback, timeout) combination is added
            // multiple times, but that might be the caller's intent.
            LOG("add scheduled callback at " << scheduled->m_timeout);
            m_scheduled_callbacks.insert(scheduled);
          }
          else {
            // When deleting, we need to delete *all* (callback, timeout)
            // combinations that match. That might not be what the caller
            // intends, but we have no way of distinguishing between them.
            LOG("remove scheduled callback");
            auto & index = m_scheduled_callbacks.get<pdt::callback_tag>();
            auto range = index.equal_range(scheduled->m_callback);
            index.erase(range.first, range.second);
            delete scheduled;
          }
        }
        break;

      case pdt::CB_ENTRY_IO:
      case pdt::CB_ENTRY_USER:
      default:
        // TODO don't know what to do here.
        break;

    }
  }
}



void
scheduler::scheduler_impl::process_scheduled_callbacks(pdu::usec_t const & now,
    std::vector<detail::scheduled_callback_entry *> & to_schedule)
{
  LOG("scheduled callbacks at: " << now);

  // Scheduled callbacks are due if their timeout is older than now(). That's
  // the simplest way to deal with them.
  auto & index = m_scheduled_callbacks.get<pdt::timeout_tag>();
  auto end = index.upper_bound(now);

  for (auto iter = index.begin() ; iter != end ; ++iter) {
    LOG("scheduled callback expired at " << now);
    if (0 == (*iter)->m_count) {
      // If it's a one shot event, we want to *move* it into the to_schedule
      // vector thereby granting ownership to the worker that picks it up.
      LOG("one-shot callback, handing over to worker");
      to_schedule.push_back(*iter);
    }
    else {
      // Depending on whether the entry gets rescheduled (more repeats) or not
      // (last invocation), we either *copy* or *move* the entry into the
      // to_schedule vector.
      LOG("interval callback, handing over to worker & rescheduling");
      if ((*iter)->m_count > 0) {
        --((*iter)->m_count);
      }

      if (0 == (*iter)->m_count) {
        // Last invocation; can *move*
        LOG("last invocation");
        to_schedule.push_back(*iter);
      }
      else {
        // More invocations to come; must *copy*
        (*iter)->m_timeout += (*iter)->m_interval;
        to_schedule.push_back(new pdt::scheduled_callback_entry(**iter));
      }
    }
  }

  // At this point, to_schedule contains everything that should go into the
  // out queue, but some of the entries might still be in m_scheduled_callbacks.
  // Those entries changed their timeout, though, so if we extract a new range
  // with the exact same parameters, we should arrive at the list of entries to
  // remove from m_scheduled_callbacks.
  end = index.upper_bound(now);
  index.erase(index.begin(), end);
}



void
scheduler::scheduler_impl::main_scheduler_loop()
{
  boost::this_thread::disable_interruption boost_interrupt_disabled;
  LOG("CPUS: " << boost::thread::hardware_concurrency());

  while (m_main_loop_continue) {
    process_in_queue();

    // get events
    pdu::sleep(pdu::from_msec(20));
    // TODO

    // Process all callbacks that want to be invoked now. Since we can't have
    // workers access the same entries we may still have in our multi-index
    // containers, we'll collect callbacks into a local vector first, and add
    // those entries to the out queue later.
    // The scheduler relinquishes ownership over entries in the to_schedule
    // vector to workers.
    pdu::usec_t now = pdu::now();
    std::vector<pdt::scheduled_callback_entry *> to_schedule;

    process_scheduled_callbacks(now, to_schedule);
    // TODO

    // After callbacks of all kinds have been added to to_schedule, we can push
    // those entries to the out queue and wake workers.
    if (!to_schedule.empty()) {
      m_out_queue.push_range(to_schedule.begin(), to_schedule.end());

      // We need to interrupt the worker pipe more than once, in order to wake
      // up multiple workers. But we don't want to interrupt the pipe more than
      // there are workers or jobs, either.
      size_t interrupts = std::min(to_schedule.size(), m_workers.size());
      while (interrupts--) {
        LOG("interrupting worker pipe");
        detail::interrupt(m_worker_pipe);
      }
    }
  }

  LOG("scheduler main loop terminated.");
}


} // namespace packetflinger
