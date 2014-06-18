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
 * This software is distributed on an "AS IS" BASIS, WITHOUT ANY WARRANTY
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.
 **/
#include <packeteer/detail/connector_pipe.h>

namespace packeteer {
namespace detail {

connector_pipe::connector_pipe()
  : m_pipe(nullptr)
{
}



connector_pipe::~connector_pipe()
{
  delete m_pipe;
}



error_t
connector_pipe::create_pipe()
{
  if (m_pipe) {
    return ERR_INITIALIZATION;
  }

  try {
    m_pipe = new pipe();
  } catch (exception const & ex) {
    return ex.code();
  } catch (...) {
    return ERR_UNEXPECTED;
  }

  return ERR_SUCCESS;
}



error_t
connector_pipe::bind()
{
  return create_pipe();
}



bool
connector_pipe::bound() const
{
  return (nullptr != m_pipe);
}



error_t
connector_pipe::connect()
{
  return create_pipe();
}



bool
connector_pipe::connected() const
{
  return (nullptr != m_pipe);
}



int
connector_pipe::get_read_fd() const
{
  if (!m_pipe) {
    return -1;
  }
  return m_pipe->get_read_fd();
}



int
connector_pipe::get_write_fd() const
{
  if (!m_pipe) {
    return -1;
  }
  return m_pipe->get_write_fd();
}



error_t
connector_pipe::read(void * buf, size_t bufsize, size_t & bytes_read)
{
  if (!m_pipe) {
    return ERR_INITIALIZATION;
  }
  return m_pipe->read(static_cast<char *>(buf), bufsize, bytes_read);
}



error_t
connector_pipe::write(void const * buf, size_t bufsize, size_t & bytes_written)
{
  if (!m_pipe) {
    return ERR_INITIALIZATION;
  }
  return m_pipe->write(static_cast<char const *>(buf), bufsize, bytes_written);
}



error_t
connector_pipe::close()
{
  delete m_pipe;
  return ERR_SUCCESS;
}

}} // namespace packeteer::detail
