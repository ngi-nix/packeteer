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
#ifndef PACKETEER_PIPE_H
#define PACKETEER_PIPE_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer/packeteer.h>

#include <packeteer/error.h>

namespace packeteer {

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

} // namespace packeteer

#endif // guard