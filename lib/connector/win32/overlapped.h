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
#ifndef PACKETEER_CONNECTOR_WIN32_OVERLAPPED_H
#define PACKETEER_CONNECTOR_WIN32_OVERLAPPED_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer.h>

#include <packeteer/handle.h>
#include <packeteer/scheduler/events.h>

#include "../../net/netincludes.h"

namespace packeteer::detail::overlapped {

/**
 * Overlapped I/O requires context for each I/O operation until it has
 * completed.
 *
 * The main piece is an OVERLAPPED structure. Read and write operations
 * need to lock the buffer they're using, and cannot re-use them for anything
 * else until the operation completed. Lastly, for waiting on overlapped
 * events, we need a handle per overlapped context.
 *
 * This is different semantics from POSIX read()/write():
 * - POSIX write() just tries to write the same buffer until it succeeds.
 *   To simulate this, we need to treat writes of the same buffer to the
 *   same handle as checks on whether the I/O operation has completed.
 *   The difference is once scheduled, the operation should eventually
 *   succeed, whereas POSIX waits for the user to try again. The best
 *   we can do is try and cancel pending writes should the user decide to
 *   write something else on the same handle.
 * - POSIX read() effectively copies from OS buffers to userspace buffers
 *   when an I/O operation completes. To simulate that, we need to allocate
 *   and pass a operation-specific buffer to ReadFile(), then copy it over
 *   on success.
 *
 * In either case, it seems we have to copy buffers in order to simulate
 * POSIX style effectively, which we need to do for a consistent API.
 *
 * This file contains functionality for handling this in a way that is not
 * tied to any specific connector.
 *
 * The manager needs to be instanciated once per HANDLE used in
 * overlapped I/O. Sharing a connector between multiple threads would either
 * require an manager per thread, or some kind of synchronized
 * access to the manager.
 *
 * A manager per thread is not a great idea; if you schedule two
 * reads on two different threads, each manager would reserve an
 * OVERLAPPED structure for the read. But if there is only enough data for
 * one read, the other's reservation stays unused and effectively leaks memory.
 *
 * It's better to either lock access to the manager so that does
 * not happen, or duplicate the handle, and give each handle its own manager.
 * TODO: see how that works with the scheduler, etc.
 */

/**
 * Overlapped operation types.
 *
 * Re-uses some PEV_IO_* values, because that simplifies the scheduler somewhat.
 */
enum PACKETEER_PRIVATE io_type : uint8_t
{
  CONNECT     = PEV_IO_OPEN,
  DISCONNECT  = PEV_IO_CLOSE,
  READ        = PEV_IO_READ,
  WRITE       = PEV_IO_WRITE,
};


/**
 * Overlapped context
 */
struct PACKETEER_PRIVATE io_context
{
  OVERLAPPED overlapped;        // The overlapped structure
  HANDLE *   handle = nullptr;  // The file, etc. for which we manage context.
                                // Pointer, because the application owns it.

  io_type    type;              // Buffers are only used for READ/WRITE

  void *     buf = nullptr;     // Reserved buffer for this context. Consider
  size_t     buflen = 0;        // this owned by the Manager class.

  size_t     source_sig = 0;    // For writes, contains a signature of
                                // the content being written.
};


/**
 * I/O action to take with the given context.
 * Either check the progress on a pending operation, or schedule an operation.
 * The operation type and its parameters are part of the context.
 */
enum PACKETEER_PRIVATE io_action : int8_t
{
  CHECK_PROGRESS = 0,
  SCHEDULE       = 1,
};


/**
 * Context IDs
 */
using context_id = size_t;


/**
 * Action callback:
 *
 * Requesting a context for any I/O operation calls this callback, up to N
 * times.
 *
 * - The io_action determines what kind of action to take.
 * - The io_context provides all information for the action.
 *
 * Return values are as follows:
 * - Return ERR_SUCCESS if the action was *completed* successfully. For
 *   a CHECK_PROGRESS action that means the operation completed.
 * - Return ERR_ASYNC if:
 *   - a CHECK_PROGRESS action was successful, but the operation was not
 *     yet completed.
 *   - a SCHEDULE was successful, but the operation was not yet completed.
 * - Return ERR_ABORTED to immediately cancel the schedule_overlapped()
 *   function.
 * - Returning any other error code is treated like ERR_ABORTED, but if
 *   possible the error code is passed on to the caller.
 */
using request_callback = std::function<
  error_t (io_action, io_context &)
>;


/**
 * The Manager class manages allocation and de-allocation of all overlapped
 * contexts, handles, etc.
 */
class PACKETEER_PRIVATE manager
{
public:
  /**
   * Create an overlapped context manager with the initial number of overlapped
   * contexts. The grow_by parameter specifies by how much the number of contexts
   * should grow when there are no free ones available. Specify -1 for grow_by to
   * double the current size, and 0 to disable growing altogether.
   */
  manager(size_t initial = PACKETEER_IO_NUM_OVERLAPPED,
      ssize_t grow_by = PACKETEER_IO_OVERLAPPED_GROW_BY);
  ~manager();


  /**
   * Request a context for the I/O type on the given handle.
   *
   * Because it's the only efficient approach with multiple I/O contexts,
   * the function will also search the existing contexts for similar
   * requests.
   *
   * Depending on the I/O type, the callback may be invoked multiple times
   * with different I/O actions for the caller to perform. The ideal situation
   * is for the callback to be invoked exactly once with an I/O action matching
   * the I/O type - that's when the context is set up for scheduling the
   * operation.
   *
   * Other actions may include checking on another context whether it's
   * completed.
   *
   * Any error the callback returns immediately leads to
   * schedule_overlapped() to abort and return the same error. In this way,
   * the caller has full control over when to abort actions.
   *
   * Errors schedule_overlapped() may return include:
   * - ERR_SUCCESS:
   *   A pending operation *completed*, and that's the only thing to be done.
   *   This may happen e.g. when attempting to schedule the same CONNECT or WRITE
   *   as done at some previous point.
   * - ERR_ASYNC:
   *   This should be the normal case, i.e. the overlapped I/O operation was
   *   scheduled, but has not yet completed.
   * - ERR_INVALID_VALUE:
   *   - for CONNECT, when other operations for the same handle are still
   *     pending.
   *   - for WRITE, when the source buffer or length are not provided.
   * - ERR_OUT_OF_MEMORY:
   *   Could not allocate a context for this operation.
   * - ERR_REPEAT_ACTION:
   *   - for WRITE, when the same write has already been scheduled. This is
   *     largely identical to handle as ERR_ASYNC.
   * - ERR_ABORTED and others:
   *   A callback has determined that the schedule_overlapped() function
   *   should error out prematurely.
   */
  error_t schedule_overlapped(HANDLE * handle, io_type type,
      request_callback && callback,
      size_t buflen = -1,
      void * source = nullptr);

  /**
   * Cancel I/O requests for the given handle. We don't distinguish further,
   * because that does not seem very useful.
   */
  error_t cancel(HANDLE * handle);

  /**
   * Cancel all I/O requests.
   */
  void cancel_all();

private:

  // All private functions are *unlocked*. Lock in the public functions.
  bool grow();

  error_t schedule_connect(HANDLE * handle,
      request_callback && callback);

  error_t schedule_read(HANDLE * handle,
      request_callback && callback,
      size_t buflen);

  error_t schedule_write(HANDLE * handle,
      request_callback && callback,
      size_t buflen,
      void * source);

  size_t signature(void * source, size_t buflen);

  void initialize(context_id id);

  error_t free_on_success(context_id id, error_t code);

  size_t                  m_initial;
  ssize_t                 m_grow_by;

  std::vector<io_context> m_contexts;
  std::list<context_id>   m_order;
};

} // namespace packeteer::detail::overlapped

#endif // guard
