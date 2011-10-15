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
#ifndef PACKETFLINGER_TYPES_H
#define PACKETFLINGER_TYPES_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packetflinger/packetflinger.h>


/*****************************************************************************
 * Some data types can come from multiple different sources and/or with
 * slightly different properties. This file provides convenience typedefs for
 * the most appropriate type found.
 **/

/*****************************************************************************
 * shared_ptr
 **/
#if defined(STDCXX_0X_HEADERS)
#  include <memory>
#  define PACKETFLINGER_SHARED_PTR_NAMESPACE std
#elif defined(STDCXX_TR1_HEADERS)
#  include <tr1/memory>
#  define PACKETFLINGER_SHARED_PTR_NAMESPACE std::tr1
#elif defined(HAVE_BOOST_SHARED_PTR_HPP)
#  include <boost/shared_ptr.hpp>
#  include <boost/pointer_cast.hpp>
#  include <boost/enable_shared_from_this.hpp>
#  define PACKETFLINGER_SHARED_PTR_NAMESPACE boost
#else
#  error No shared_ptr implementation found.
#endif

namespace packetflinger {

using PACKETFLINGER_SHARED_PTR_NAMESPACE ::shared_ptr;
using PACKETFLINGER_SHARED_PTR_NAMESPACE ::weak_ptr;
using PACKETFLINGER_SHARED_PTR_NAMESPACE ::enable_shared_from_this;

using PACKETFLINGER_SHARED_PTR_NAMESPACE ::static_pointer_cast;
using PACKETFLINGER_SHARED_PTR_NAMESPACE ::dynamic_pointer_cast;
using PACKETFLINGER_SHARED_PTR_NAMESPACE ::const_pointer_cast;

} // namespace packetflinger

#undef PACKETFLINGER_SHARED_PTR_NAMESPACE

/*****************************************************************************
 * Find a suitable hash map type, and fall back to a tree map if none is to be
 * found.
 *
 * Unfortunately, template aliases aren't widely supported yet, so we'll have
 * to fall back on a preprocessor solution.
 **/
#if defined(HAVE_UNORDERED_MAP)
#  include <unordered_map>
#  define packetflinger_hash_map std::unordered_map
#elif defined(HAVE_TR1_UNORDERED_MAP)
#  include <tr1/unordered_map>
#  define packetflinger_hash_map std::tr1::unordered_map
#elif defined(HAVE_EXT_HASH_MAP)
#  include <ext/hash_map>
#  define packetflinger_hash_map std::hash_map
#elif defined(HAVE_GNUCXX_HASHMAP)
#  include <ext/hash_map>
#  define packetflinger_hash_map __gnu_cxx::hash_map
#else
#  include <map>
#  define packetflinger_hash_map std::map
#endif

#endif // guard
