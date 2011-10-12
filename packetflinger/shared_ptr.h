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
#ifndef PACKETFLINGER_SHARED_PTR_H
#define PACKETFLINGER_SHARED_PTR_H

#include <packetflinger/packetflinger-config.h>

/**
 * Defines the following classes, either from the TR1 namespace (if available),
 * or from boost (if availble).
 *
 * It makes sense to use packetflinger's definitions (if you're using
 * packetflinger anyway), as they'll use defintions from the std namespace as
 * they become available.
 **/


// Prefer the TR1 definition
#if defined(HAVE_TR1_MEMORY)

#include <tr1/memory>
namespace packetflinger {
using std::tr1::shared_ptr;
using std::tr1::weak_ptr;
using std::tr1::enable_shared_from_this;

using std::tr1::static_pointer_cast;
using std::tr1::dynamic_pointer_cast;
using std::tr1::const_pointer_cast;

} // namespace packetflinger

#else // HAVE_TR1_MEMORY

// Only if tr1::shared_ptr is not available, use boost::shared_ptr
#if defined(HAVE_BOOST_SHARED_PTR_HPP)

#include <boost/shared_ptr.hpp>
#include <boost/pointer_cast.hpp>
#include <boost/enable_shared_from_this.hpp>

namespace packetflinger {
using boost::shared_ptr;
using boost::weak_ptr;
using boost::enable_shared_from_this;

using boost::static_pointer_cast;
using boost::dynamic_pointer_cast;
using boost::const_pointer_cast;

} // namespace packetflinger

#else // HAVE_BOOST_SHARED_PTR_HPP

// Can't really build this, error out.
#error Neither tr1::shared_ptr nor boost::shared_ptr available.

#endif // HAVE_BOOST_SHARED_PTR_HPP

#endif // HAVE_TR1_MEMORY

#endif // guard
