/**
 * This file is part of packetflinger.
 *
 * Author(s): Jens Finkhaeuser <unwesen@users.sourceforge.net>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

    int ret = ::select(0, NULL, NULL, NULL, &tv);
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
  ::gettimeofday(&tv, NULL);
  return from_sec(tv.tv_sec) + tv.tv_usec;
}

}} // namespace packetflinger::duration
