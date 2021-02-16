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
 * This software is distributed on an "AS IS" BASIS, WITHOUT ANY WARRANTY
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.
 **/
#include <build-config.h>

#include <string>

#include <unistd.h>

#include <liberate/net/url.h>
#include <liberate/string/util.h>

#include <packeteer/registry.h>

#include "../lib/connector/posix/fd.h"
#include "../lib/connector/posix/common.h"
#include "../lib/connector/util.h"
#include "../lib/macros.h"

namespace packeteer::ext {

/**
 * Simple wrapper around already opened file descriptor.
 */
struct connector_filedesc : public ::packeteer::detail::connector_common
{
public:
  connector_filedesc(int fd, connector_options const & options)
    : connector_common(options)
    , m_fd(fd)
  {
  }

  virtual ~connector_filedesc() {};

  virtual error_t listen()
  {
    return ERR_SUCCESS;
  }


  virtual bool listening() const
  {
    return true;
  }

  virtual error_t connect()
  {
    return ERR_SUCCESS;
  }

  virtual bool connected() const
  {
    return true;
  }

  virtual connector_interface * accept(liberate::net::socket_address &)
  {
    return this;
  }

  virtual handle get_read_handle() const
  {
    return m_fd;
  }

  virtual handle get_write_handle() const
  {
    return m_fd;
  }

  virtual error_t close()
  {
    return ERR_UNSUPPORTED_ACTION;
  }

  /***************************************************************************
   * Setting accessors
   **/
  virtual bool is_blocking() const
  {
    bool blocking = false;
    auto err = detail::get_blocking_mode(m_fd, blocking);
    if (ERR_SUCCESS != err) {
      throw new packeteer::exception(err, "Could not determine blocking mode of FD!");
    }
    return blocking;
  }

private:
  int m_fd;
};


namespace {

packeteer::connector_interface *
filedesc_creator(liberate::net::url const & url,
    packeteer::connector_type const &,
    packeteer::connector_options const & options,
    packeteer::registry::connector_info const * info)
{
  // Parse URL path
  if (url.path[0] != '/') {
    ELOG("Invalid path format.");
    return nullptr;
  }
  auto path = url.path.substr(1);

  // First try to parse the path as a number
  int fd = -1;
  try {
    fd = std::stoi(path);
  } catch (std::exception const & ex) {
    DLOG("Could not convert file descriptor: " << ex.what());
  }

  // If that did not work, check for special strings
  if (-1 == fd) {
    auto lower = liberate::string::to_lower(path);
    if (lower == "stdin") {
      fd = STDIN_FILENO;
    }
    else if (lower == "stdout") {
      fd = STDOUT_FILENO;
    }
    else if (lower == "stderr") {
      fd = STDERR_FILENO;
    }
  }

  // If that didn't yield anything, abort.
  if (-1 == fd) {
    ELOG("Could not determine file descriptor to use.");
    return nullptr;
  }

  // Sanitize options
  auto opts = detail::sanitize_options(options, info->default_options,
      info->possible_options);

  // Set FD to blocking/non-blocking, depending on what the options say.
  // XXX: This alters the original FD's behaviour, which may not be desired.
  auto err = detail::set_blocking_mode(fd, opts & CO_BLOCKING);
  if (ERR_SUCCESS != err) {
    ELOG("Could not set blocking mode of FD!");
    return nullptr;
  }

  // All good!
  return new connector_filedesc(fd, opts);
}

} // private namespace


PACKETEER_API
error_t
register_connector_filedesc(std::shared_ptr<packeteer::api> api,
    packeteer::connector_type register_as /* = CT_USER */)
{
  packeteer::registry::connector_info info{
    register_as,
    CO_STREAM|CO_BLOCKING,
    CO_STREAM|CO_BLOCKING|CO_NON_BLOCKING,
    filedesc_creator,
  };

  api->reg().add_scheme("filedesc", info);
  api->reg().add_scheme("fd", info);

  return ERR_SUCCESS;
}


} // namespace packeteer::ext
