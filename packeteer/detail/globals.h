/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2017 Jens Finkhaeuser.
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
#ifndef PACKETEER_DETAIL_GLOBALS_H
#define PACKETEER_DETAIL_GLOBALS_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

/*****************************************************************************
 * *Internal* global definitions.
 **/

/**
 * ::listen() has a backlog parameter that determines how many pending
 * connections the kernel should keep. Widely discussed as common is the value
 * 128, so we'll use that.
 **/
#define PACKETEER_LISTEN_BACKLOG  128


/**
 * Maximum number of events to read with Epoll/KQueue at a single call.
 **/
#define PACKETEER_EPOLL_MAXEVENTS   256
#define PACKETEER_KQUEUE_MAXEVENTS  PACKETEER_EPOLL_MAXEVENTS

#endif // guard
