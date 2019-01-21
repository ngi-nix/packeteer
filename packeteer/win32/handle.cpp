/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2019 Jens Finkhaeuser.
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

#include <packeteer/handle.h>

namespace packeteer {

namespace {

} // anonymous namespace


error_t
set_blocking_mode(handle::sys_handle_t const & fd, bool state /* = false */)
{
	// TODO
  return ERR_SUCCESS;
}


error_t
get_blocking_mode(handle::sys_handle_t const & fd, bool & state)
{
	// TODO
  return ERR_SUCCESS;
}



} // namespace packeteer
