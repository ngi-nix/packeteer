/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
 * Copyright (c) 2012-2014 Unwesen Ltd.
 * Copyright (c) 2015-2020 Jens Finkhaeuser.
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
#include <packeteer.h>

#include <chrono>

#include <stdlib.h>

#include <packeteer/registry.h>
#include <packeteer/resolver.h>

#include "macros.h"

namespace packeteer {

struct api::api_impl
{
  api_impl(std::weak_ptr<api> api)
    : reg{api}
    , res{api}
  {
  }

  ::liberate::api       liberate = {};
  ::packeteer::registry reg;
  ::packeteer::resolver res;
};



std::shared_ptr<api>
api::create()
{
  auto ret = std::shared_ptr<api>(new api());
  ret->init(ret);
  return ret;
}



api::api()
{
}



void
api::init(std::weak_ptr<api> self)
{
  m_impl = std::make_unique<api_impl>(self);

  // Initialize random seed used in connector construction.
  auto now = std::chrono::steady_clock::now().time_since_epoch();
  srand(now.count());
}



api::~api()
{
}



::packeteer::registry &
api::reg()
{
  return m_impl->reg;
}



::packeteer::registry &
api::registry()
{
  return m_impl->reg;
}



::packeteer::resolver &
api::res()
{
  return m_impl->res;
}



::packeteer::resolver &
api::resolver()
{
  return m_impl->res;
}



::liberate::api &
api::liberate()
{
  return m_impl->liberate;
}



} // namespace packeteer
