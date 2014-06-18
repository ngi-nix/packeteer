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
  /**
   * Constructor; if the block parameter is given, read and write calls will 
   * block until the specified buffer size is read or written respectively.
   **/
  pipe(bool block = true);
  ~pipe();

  /**
   * Read from the pipe into the given buffer, or write to the pipe from the
   * given buffer. Read or write at most bufsize bytes. The actual amount read
   * or written is returned in the last parameter.
   **/
  error_t write(char const * buf, size_t bufsize, size_t & bytes_written);
  error_t read(char * buf, size_t bufsize, size_t & bytes_read);

  /**
   * Get read and write file descriptors for use with the scheduler class.
   **/
  int get_read_fd() const;
  int get_write_fd() const;

private:
  int m_fds[2];
};

} // namespace packeteer

#endif // guard
