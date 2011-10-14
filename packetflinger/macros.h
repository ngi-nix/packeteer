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
#ifndef PACKETFLINGER_MACROS_H
#define PACKETFLINGER_MACROS_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

/**
 * Stringify the symbol passed to PACKETFLINGER_SPRINGIFY()
 **/
#define PACKETFLINGER_STRINGIFY(n) PACKETFLINGER_STRINGIFY_HELPER(n)
#define PACKETFLINGER_STRINGIFY_HELPER(n) #n

/**
 * Define logging symbols if -DDEBUG is specified (or -DNDEBUG isn't).
 **/
#if defined(DEBUG) && !defined(NDEBUG)
#include <iostream>
#define LOG(msg) std::cerr << "DEBUG: " << msg << std::endl;
#else
#define LOG(msg)
#endif

#endif // guard
