/**
 * This file is part of packetflinger.
 *
 * Author(s): Jens Finkhaeuser <jens@unwesen.co.uk>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 * Copyright (c) 2012 Unwesen Ltd.
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
#include <packetflinger/duration.h>

#if defined(HAVE_SYS_SELECT_H)
#  include <sys/select.h>
#elif defined(HAVE_SYS_TIME_H) && defined(HAVE_SYS_TYPES_H) && defined(HAVE_UNISTD_H)
#  include <sys/types.h>
#  include <unistd.h>
#else
#  error Cannot compile on this system.
#endif

#include <sys/time.h>
#include <errno.h>

#include <packetflinger/error.h>

namespace packetflinger {
namespace duration {


void sleep(usec_t const & interval)
{
  usec_t start = now();
  usec_t remain = interval;

  do {
    ::timeval tv;

    tv.tv_sec = to_sec(remain);
    tv.tv_usec = remain - from_sec(to_sec(remain));

    int ret = ::select(0, nullptr, nullptr, nullptr, &tv);
    if (0 == ret) {
      break;
    }

    // Error handling really exists only because of EINTR - in which case we
    // may want to sleep again, for the remainder of the specified interval.
    if (-1 == ret) {
      switch (errno) {
        case EINTR:
          remain -= now() - start;
          break;

        case EINVAL:
          throw exception(ERR_INVALID_VALUE);
          break;

        case ENOMEM:
          throw exception(ERR_OUT_OF_MEMORY);
          break;

        default:
          throw exception(ERR_UNEXPECTED);
          break;
      }
    }

    // ret is neither 0 nor -1 - very unexpected.
    throw exception(ERR_UNEXPECTED);

  } while (remain > 0);
}



usec_t now()
{
  ::timeval tv;
  ::gettimeofday(&tv, nullptr);
  return from_sec(tv.tv_sec) + tv.tv_usec;
}

}} // namespace packetflinger::duration
