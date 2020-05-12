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

#include "../../macros.h"

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
manager::schedule_overlapped(HANDLE handle, io_type type,
    request_callback && callback,
    size_t buflen /* = -1*/,
    void * source /* = nullptr*/)
{
  std::lock_guard<std::mutex> lock(m_mutex);

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
manager::schedule_connect(HANDLE handle,
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
        found = i;
        connecting = true;
        break;
      }
      DLOG("Other pending operations for handle, cannot connect.");
      return ERR_INVALID_VALUE;
    }

    // If we found a free one, remember the index.
    if (INVALID_HANDLE_VALUE == context.handle && found < 0) {
      found = i;
    }
  }

  // If we're already connecting, let's check the progress on the operation.
  if (connecting) {
    DLOG("Connect already scheduled for handle, check progress.");
    auto & context = m_contexts[found];
    return free_unless_async(found, callback(CHECK_PROGRESS, context));
  }

  // We're good, so let's see if we found a free slot. If not, we need to grow.
  // If we grow, the next free slot is the current size.
  if (found < 0) {
    DLOG("No free slot found, try growing the pool.");
    found = m_contexts.size();
    if (!grow()) {
      DLOG("Cannot grow pool!");
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
  DLOG("Invoking callback to connect handle; currently " << m_order.size()
      << " operations are pending in total.");

  // Callback for the actual connection.
  return free_unless_async(found, callback(SCHEDULE, context));
}
 


error_t
manager::schedule_read(HANDLE handle,
    request_callback && callback,
    size_t buflen)
{
  // When reading, we want to check if there are other reads in flight. If so,
  // their status should be the result of the function. If the in-flight read
  // is still pending, let the caller try again later. If it's done, let the
  // caller schedule the next read. etc.
  auto order_copy = m_order; // So we can run free_unless_async() within the loop
  for (auto id : order_copy) {
    auto & context = m_contexts[id];
    if (context.handle != handle || context.type != READ) {
      continue;
    }

    // If we have a pending, zero-byte read, then we have to tread a bit
    // differently from other reads. We want to keep it pending until a
    // "regular" read is scheduled.
    if (context.buflen == 0) {
      if (buflen == 0) {
        // A zero-byte read was pending and scheduled. In this case, we
        // just pretend the second of those was immediately successful.
        return ERR_SUCCESS;
      }

      // A zero-byte read was pending, and a regular read was scheduled.
      // This is good; we want to cancel the zero-byte read, and let the
      // loop continue in the hopes we'll find a slot for the regular
      // read.
      CancelIoEx(context.handle, &context);
      free_unless_async(id, ERR_SUCCESS);
      continue;
    }

    // We have a regular read on this handle already, check progress and return the
    // result.
    return free_unless_async(id, callback(CHECK_PROGRESS, context));
  }

  // Nothing scheduled, so let's find the first free slot; grow if necessary.
  ssize_t found = -1;
  for (size_t i = 0 ; i < m_contexts.size() ; ++i) {
    auto & context = m_contexts[i];
    if (INVALID_HANDLE_VALUE == context.handle) {
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
  if (buflen <= 0) {
    // Schedule read, even if we don't want any results.
    context.buflen = 0;
    context.buf = new char[1];
  }
  else {
    // Schedule regular read.
    context.buflen = buflen;
    context.buf = new char[buflen];
  }
  context.source_sig = 0;

  m_order.push_back(found);

  return free_unless_async(found, callback(SCHEDULE, context));
}
 


error_t
manager::schedule_write(HANDLE handle,
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
    // FIXME it may be worthwile to allow zero-byte writes, e.g. if we can use
    //       it in the same way we're using zero-byte reads in the IOCP
    //       subsystem.
    DLOG("Can't write without anything to write.");
    return ERR_INVALID_VALUE;
  }
  source_sig = signature(source, buflen);
  DLOG("Source signature for WRITE on handle " << std::hex << handle
      << " is: " << source_sig << std::dec);

  // After that, we can check existing writes.
  bool found_same = false;
  auto order_copy = m_order; // So we can run free_unless_async() in the loop
  for (auto id : order_copy) {
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
    auto err = free_unless_async(id, callback(CHECK_PROGRESS, context));
    switch (err) {
      // Continue scheduling writes until we get errors.
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
    if (INVALID_HANDLE_VALUE == context.handle) {
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

  return free_unless_async(found, callback(SCHEDULE, context));
}



void
manager::initialize(context_id id)
{
  auto & ctx = m_contexts[id];
  ctx.handle = INVALID_HANDLE_VALUE;

  delete [] ctx.buf;
  ctx.buf = nullptr;
  ctx.buflen = 0;

  ctx.source_sig = 0;
}



error_t
manager::free_unless_async(context_id id, error_t code)
{
  // Async results mean we have to revisit this context.
  if (code == ERR_ASYNC) {
    return code;
  }

  initialize(id);
  m_order.remove(id);

  DLOG("Freed "
      << (code == ERR_SUCCESS ? "completed" : "errored")
      << " slot " << id << "; currently "<< m_order.size()
      << " operations are pending in total.");

  return code;
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



ssize_t
manager::read_scheduled() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  for (auto id : m_order) {
    auto & context = m_contexts[id];
    if (context.handle == INVALID_HANDLE_VALUE) {
      continue;
    }
    if (context.type != READ) {
      continue;
    }

    return context.buflen;
  }

  return -1;
}



void
manager::cancel_reads()
{
  std::lock_guard<std::mutex> lock(m_mutex);

  // There is only ever one read scheduled.
  ssize_t found_id = -1;
  for (auto id : m_order) {
    auto & context = m_contexts[id];
    if (context.type == READ) {
      found_id = id;
      break;
    }
  }

  if (found_id < 0) {
    DLOG("Cancelling reads was requested, but no reads found.");
    return;
  }

  // Cancel this read.
  auto & context = m_contexts[found_id];
  auto ret = CancelIoEx(context.handle, &context);
  if (!ret) {
    ERRNO_LOG("Unexpected error cancelling READ operations.");
  }

  // Free slot
  initialize(found_id);
  m_order.remove(found_id);

  DLOG("Cancelled all READs for handle; currently "<< m_order.size()
      << " operations are pending in total.");
}




void
manager::cancel_all()
{
  std::lock_guard<std::mutex> lock(m_mutex);

  // First find all unique handles.
  std::set<HANDLE> unique;
  for (size_t i = 0 ; i < m_contexts.size() ; ++i) {
    if (INVALID_HANDLE_VALUE != m_contexts[i].handle) {
      unique.insert(m_contexts[i].handle);
    }
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

  DLOG("Cancelled all pending I/O on " << unique.size() << " handle(s).");
}



} // namespace packeteer::detail::overlapped
