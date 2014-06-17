/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@unwesen.co.uk>
 *
 * Copyright (c) 2014 Unwesen Ltd.
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

#include <arpa/inet.h>

#if defined(linux) || defined(__gnu_linux__) || defined(__linux__) || defined(__linux)
#include <linux/if.h>
#include <linux/if_tun.h>
#endif

#endif // guard
