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
#ifndef PACKETFLINGER_PIPE_H
#define PACKETFLINGER_PIPE_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packetflinger/packetflinger.h>

#include <packetflinger/error.h>

namespace packetflinger {

/*****************************************************************************
 * Simple abstraction for anonymous pipes.
 **/
class pipe
{
public:
  pipe();
  ~pipe();

  error_t write(char const * buf, size_t bufsize);
  error_t read(char * buf, size_t bufsize, size_t & amount);

  int get_read_fd();
  int get_write_fd();

private:
  int m_fds[2];
};

} // namespace packetflinger

#endif // guard
