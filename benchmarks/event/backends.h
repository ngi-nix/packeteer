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
#ifndef P7R_BENCH_EVENT_BACKENDS_H
#define P7R_BENCH_EVENT_BACKENDS_H

#include <iostream>
#include <vector>
#include <functional>

enum class backends
{
  PACKETEER = 0,
  LIBEVENT,
  LIBEV,
  LIBUV,
  ASIO,
};

std::ostream & operator<<(std::ostream & os, backends const & b);



struct options
{
  backends  backend = backends::PACKETEER;
  size_t    conns = 100;
  size_t    active = 1;
  size_t    writes = 100;
  bool      verbose = false;
};



struct backend_ops
{
  virtual ~backend_ops() {};

  virtual void init(options const &) {};
  // TODO
};



void register_backend(backends b,
    std::string const & name,
    std::vector<std::string> const & matches,
    backend_ops * ops);


#endif // P7R_BENCH_EVENT_BACKENDS_H
