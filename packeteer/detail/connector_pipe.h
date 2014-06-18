/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@unwesen.co.uk>
 *
 * Copyright (c) 2014 Unwesen Ltd.
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
#ifndef PACKETEER_DETAIL_CONNECTOR_PIPE_H
#define PACKETEER_DETAIL_CONNECTOR_PIPE_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer/packeteer.h>

#include <packeteer/detail/connector.h>

#include <packeteer/pipe.h>

namespace packeteer {
namespace detail {

/**
 * Wrapper around the pipe class.
 **/
struct connector_pipe : public ::packeteer::detail::connector
{
public:
  connector_pipe(bool block = false);
  ~connector_pipe();

  error_t bind();
  bool bound() const;

  error_t connect();
  bool connected() const;

  int get_read_fd() const;
  int get_write_fd() const;

  error_t read(void * buf, size_t bufsize, size_t & bytes_read);
  error_t write(void const * buf, size_t bufsize, size_t & bytes_written);

  error_t close();
private:
  error_t create_pipe();

  bool  m_block;
  int   m_fds[2];
};

}} // namespace packeteer::detail

#endif // guard
