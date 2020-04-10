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

#include <algorithm>
#include <set>

#include "overlapped.h"

#include ",,/../macros.h"

namespace packeteer::detail::overlapped {

manager::manager(size_t initial, ssize_t grow_by)
  : m_initial{initial}
  , m_grow_by{grow_by}
  , m_contexts{initial}
{
  if (!initial && grow_by <= 0) {
    throw exception(ERR_INVALID_VALUE, "Cannot set the initial pool size to "
        "and disable growth or set it to double.");
  }

  for (size_t i = 0 ; i < m_contexts.size() ; ++i) {
    initialize(i);
  }
}



manager::~manager()
{
  cancel_all();
}



bool
manager::grow()
{
  if (!m_grow_by) {
    return false;
  }

  size_t previous = m_contexts.size();

  size_t grow_to = previous + m_grow_by;
  if (-1 == m_grow_by) {
    grow_to = previous * 2;
  }

  m_contexts.resize(grow_to);

  return true;
}



error_t
manager::schedule_overlapped(HANDLE * handle, io_type type,
    request_callback && callback,
    size_t buflen /* = -1*/,
    void * source /* = nullptr*/)
{
  switch (type) {
    case CONNECT:
      return schedule_connect(handle, std::move(callback));

    case READ:
      return schedule_read(handle, std::move(callback),
          buflen);

    case WRITE:
      return schedule_write(handle, std::move(callback),
          buflen, source);
  }

  PACKETEER_FLOW_CONTROL_GUARD;
}



error_t
manager::schedule_connect(HANDLE * handle,
    request_callback && callback)
{
  // If there are any slots in use for this handle, return an error. Also
  // search for an empty slot, why iterate twice?
  ssize_t found = -1;
  bool connecting = false;
  for (size_t i = 0 ; i < m_contexts.size() ; ++i) {
    auto & context = m_contexts[i];

    // Error out on duplicates
    if (context.handle == handle) {
      // Special case: if this context is already attempting a connection,
      // we want to check progress on it.
      // Since we bail out on any other slot type, this must not only be the
      // only CONNECT slot, but actually the only slot in use.
      if (CONNECT == context.type) {
        LOG("Connect already scheduled for handle, check progress.");
        found = i;
        connecting = true;
        break;
      }
      LOG("Other pending operations for handle, cannot connect.");
      return ERR_INVALID_VALUE;
    }

    // If we found a free one, remember the index.
    if (!context.handle && found < 0) {
      found = i;
    }
  }

  // If we're already connecting, let's check the progress on the operation.
  if (connecting) {
    LOG("Invoking callback to check progress.");
    auto & context = m_contexts[found];
    return free_on_success(found, callback(CHECK_PROGRESS, context));
  }

  // We're good, so let's see if we found a free slot. If not, we need to grow.
  // If we grow, the next free slot is the current size.
  if (found < 0) {
    LOG("No free slot found, try growing the pool.");
    found = m_contexts.size();
    if (!grow()) {
      LOG("Cannot grow pool!");
      return ERR_OUT_OF_MEMORY;
    }
  }

  // Whether through growing or not, we've got a slot index. Initalize this
  // context to what we need, and pass it on to the callback.
  auto & context = m_contexts[found];
  context.handle = handle;
  context.type = CONNECT;
  context.buf = nullptr;
  context.buflen = 0;
  context.source_sig = 0;

  m_order.push_back(found);
  LOG("Invoking callback to connect handle; currently " << m_order.size()
      << " operations are pending in total.");

  // Callback for the actual connection.
  return free_on_success(found, callback(SCHEDULE, context));
}
 


error_t
manager::schedule_read(HANDLE * handle,
    request_callback && callback,
    size_t buflen)
{
  // Scheduling reads is fine even if other reads are scheduled. The important
  // part is that we also check progress on previous reads on the same handle.
  // And we should do so in order, until the first one that is not completed.
  for (auto id : m_order) {
    auto & context = m_contexts[id];
    if (context.handle != handle || context.type != READ) {
      continue;
    }

    // We have a read on this handle already, check progress.
    auto err = free_on_success(id, callback(CHECK_PROGRESS, context));
    switch (err) {
      case ERR_SUCCESS:
        continue;

      case ERR_ASYNC:
        break;

      default:
        return err;
    }
  }

  // Now find the first free slot; grow if necessary.
  ssize_t found = -1;
  for (size_t i = 0 ; i < m_contexts.size() ; ++i) {
    auto & context = m_contexts[i];
    if (!context.handle) {
      found = i;
      break;
    }
  }

  if (found < 0) {
    found = m_contexts.size();
    if (!grow()) {
      return ERR_OUT_OF_MEMORY;
    }
  }

  // We have a slot for the read. Use it!
  auto & context = m_contexts[found];
  context.handle = handle;
  context.type = READ;
  context.buflen = buflen ? buflen : PACKETEER_IO_BUFFER_SIZE;
  context.buf = new char[context.buflen];
  context.source_sig = 0;

  m_order.push_back(found);

  return free_on_success(found, callback(SCHEDULE, context));
}
 


error_t
manager::schedule_write(HANDLE * handle,
    request_callback && callback,
    size_t buflen,
    void * source)
{
  // When writing, we can check the progress on *all* pending writes on the
  // handle (wheras with reading, we want to keep the order).
  //
  // But if there is a write with the same source signature pending, we need
  // to know, as we can't schedule the same write again.
  //
  // So first calculate our source_sig.
  size_t source_sig = 0;
  if (!source || !buflen) {
    LOG("Can't write without anything to write.");
    return ERR_INVALID_VALUE;
  }
  source_sig = signature(source, buflen);
  LOG("Source signature for WRITE on handle " << std::hex << handle
      << " is: " << source_sig << std::dec);

  // After that, we can check existing writes.
  bool found_same = false;
  for (auto id : m_order) {
    auto & context = m_contexts[id];
    if (context.handle != handle || context.type != WRITE) {
      continue;
    }

    // If we have found the same write (source_sig), then we need to note
    // that.
    if (context.source_sig == source_sig) {
      found_same = true;
    }

    // We have a write on this handle already, check progress.
    auto err = free_on_success(id, callback(CHECK_PROGRESS, context));
    switch (err) {
      case ERR_SUCCESS:
      case ERR_ASYNC:
        continue;

      default:
        return err;
    }
  }

  // If we found the same write (same handle, same sig), we can't schedule the
  // operation again. We already checked progress above, so here we just exit.
  if (found_same) {
    return ERR_REPEAT_ACTION;
  }
  
  // Ok, we should schedule this write, so let's find a free slot.
  ssize_t found = -1;
  for (size_t i = 0 ; i < m_contexts.size() ; ++i) {
    auto & context = m_contexts[i];
    if (!context.handle) {
      found = i;
      break;
    }
  }

  if (found < 0) {
    found = m_contexts.size();
    if (!grow()) {
      return ERR_OUT_OF_MEMORY;
    }
  }

  // We have a slot for the read. Use it!
  auto & context = m_contexts[found];
  context.handle = handle;
  context.type = WRITE;
  context.buflen = buflen;
  context.buf = new char[buflen];
  ::memcpy(context.buf, source, buflen);
  context.source_sig = source_sig;

  m_order.push_back(found);

  return free_on_success(found, callback(SCHEDULE, context));
}



void
manager::initialize(context_id id)
{
  auto & ctx = m_contexts[id];
  ctx.handle = nullptr;

  delete [] ctx.buf;
  ctx.buf = nullptr;
  ctx.buflen = 0;

  ctx.source_sig = 0;
}



error_t
manager::free_on_success(context_id id, error_t code)
{
  if (ERR_SUCCESS != code) {
    return code;
  }

  initialize(id);
  m_order.remove(id);

  LOG("Freed completed slot " << id << "; currently "<< m_order.size()
      << " operations are pending in total.");

  return ERR_SUCCESS;
}



size_t
manager::signature(void * source, size_t buflen)
{
  // Called only for non-null sources, so no need to check again. The main
  // question is how much of the source to hash.
  size_t amount = buflen;
  if (PACKETEER_IO_SIGNATURE_SIZE > 0) {
    amount = std::min<size_t>(buflen, PACKETEER_IO_SIGNATURE_SIZE);
  }

  char const * converted = static_cast<char *>(source);
  std::string tmp{converted, converted + amount};

  // Hash the temporary value:
  return std::hash<std::string>{}(tmp);
}



error_t
manager::cancel(HANDLE * handle)
{
  // Cancel all I/O
  auto ret = CancelIoEx(handle, nullptr);

  std::set<context_id> to_free;
  for (size_t i = 0 ; i < m_contexts.size() ; ++i) {
    if (m_contexts[i].handle == handle) {
      to_free.insert(i);
    }
  }

  for (auto id : to_free) {
    initialize(id);
    m_order.remove(id);
  }

  LOG("Cancelled all I/O for handle; currently "<< m_order.size()
      << " operations are pending in total.");

  if (ret || GetLastError() == ERROR_NOT_FOUND) {
    return ERR_SUCCESS;
  }
  ERRNO_LOG("Unexpected error cancelling I/O operations.");
  return ERR_UNEXPECTED;
}




void
manager::cancel_all()
{
  // First find all unique handles.
  std::set<HANDLE *> unique;
  for (size_t i = 0 ; i < m_contexts.size() ; ++i) {
    unique.insert(m_contexts[i].handle);
  }

  // Cancel I/O on all handles
  for (auto handle : unique) {
    CancelIoEx(handle, nullptr);
  }

  // Finally initialize every slot
  for (size_t i = 0 ; i < m_contexts.size() ; ++i) {
    initialize(i);
  }
  m_order.clear();

  LOG("Cancelled all pending I/O.");
}



} // namespace packeteer::detail::overlapped
