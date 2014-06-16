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
#ifndef PACKETEER_TYPES_H
#define PACKETEER_TYPES_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <packeteer/packeteer.h>


/*****************************************************************************
 * Some data types can come from multiple different sources and/or with
 * slightly different properties. This file provides convenience typedefs for
 * the most appropriate type found.
 **/

/*****************************************************************************
 * shared_ptr
 **/
#if defined(PACKETEER_STDCXX_0X_HEADERS)
#  include <memory>
#  define PACKETEER_SHARED_PTR_NAMESPACE std
#elif defined(PACKETEER_STDCXX_TR1_HEADERS)
#  include <tr1/memory>
#  define PACKETEER_SHARED_PTR_NAMESPACE std::tr1
#else
#  error No shared_ptr implementation found.
#endif

namespace packeteer {

using PACKETEER_SHARED_PTR_NAMESPACE ::shared_ptr;
using PACKETEER_SHARED_PTR_NAMESPACE ::weak_ptr;
using PACKETEER_SHARED_PTR_NAMESPACE ::enable_shared_from_this;

using PACKETEER_SHARED_PTR_NAMESPACE ::static_pointer_cast;
using PACKETEER_SHARED_PTR_NAMESPACE ::dynamic_pointer_cast;
using PACKETEER_SHARED_PTR_NAMESPACE ::const_pointer_cast;

} // namespace packeteer

#undef PACKETEER_SHARED_PTR_NAMESPACE

/*****************************************************************************
 * Find a suitable hash map/set type, and fall back to a tree map/set if none
 * is to be found.
 *
 * Unfortunately, template aliases aren't widely supported yet, so we'll have
 * to fall back on a preprocessor solution.
 **/
#if defined(PACKETEER_HAVE_UNORDERED_MAP)
#  include <unordered_map>
#  define packeteer_hash_map std::unordered_map
#elif defined(PACKETEER_HAVE_TR1_UNORDERED_MAP)
#  include <tr1/unordered_map>
#  define packeteer_hash_map std::tr1::unordered_map
#elif defined(PACKETEER_HAVE_EXT_HASH_MAP)
#  include <ext/hash_map>
#  define packeteer_hash_map std::hash_map
#elif defined(PACKETEER_HAVE_GNUCXX_HASHMAP)
#  include <ext/hash_map>
#  define packeteer_hash_map __gnu_cxx::hash_map
#else
#  include <map>
#  define packeteer_hash_map std::map
#endif

#if defined(PACKETEER_HAVE_UNORDERED_SET)
#  include <unordered_set>
#  define packeteer_hash_set std::unordered_set
#elif defined(PACKETEER_HAVE_TR1_UNORDERED_SET)
#  include <tr1/unordered_set>
#  define packeteer_hash_set std::tr1::unordered_set
#elif defined(PACKETEER_HAVE_EXT_HASH_SET)
#  include <ext/hash_set>
#  define packeteer_hash_set std::hash_set
#else
#  include <set>
#  define packeteer_hash_set std::set
#endif


#endif // guard
