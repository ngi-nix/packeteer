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
#ifndef PACKETFLINGER_PACKETFLINGER_H
#define PACKETFLINGER_PACKETFLINGER_H

#ifdef __cplusplus

namespace packetflinger {

/**
 * Library context. Used to manage ownership over internal data structures.
 *
 * You can use multiple library contexts at the same time, but any IDs for
 * referencing other library objects (such as connections) are relative to
 * the context in which they were created.
 *
 * Destroying a context shuts down all activity started within the context,
 * and frees all allocated memory.
 **/
struct context;

/**
 * Creates a library context.
 * TODO parameters, configuration.
 **/
context create_context();

/**
 * TODO
 **/
void destroy_context(context & ctx);



} // namespace packetflinger

#endif

#endif // guard
