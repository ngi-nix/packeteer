/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2014 Unwesen Ltd.
 * Copyright (c) 2015-2017 Jens Finkhaeuser.
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

#ifndef PACKETEER_NET_DETAIL_SYSINCLUDES_H
#define PACKETEER_NET_DETAIL_SYSINCLUDES_H

#if defined(PACKETEER_HAVE_ARPA_INET_H)
#include <arpa/inet.h>
#endif

#if defined(PACKETEER_HAVE_NETINET_IN_H)
#include <netinet/in.h>
#endif

#if defined(PACKETEER_HAVE_LINUX_UN_H)
#  include <linux/un.h>
#else
#  include <sys/un.h>
#  define UNIX_PATH_MAX 108
#endif

#endif // guard
