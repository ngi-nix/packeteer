/**
 * This file is part of packetflinger.
 *
 * Author(s): Jens Finkhaeuser <unwesen@users.sourceforge.net>
 *
 * Copyright (c) 2011 Jens Finkhaeuser.
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
