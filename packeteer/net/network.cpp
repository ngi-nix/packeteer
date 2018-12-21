/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2014 Unwesen Ltd.
 * Copyright (c) 2015-2019 Jens Finkhaeuser.
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

#include <packeteer/net/network.h>

#include <cstring>
#include <cmath>

#include <functional>
#include <set>

#include <packeteer/types.h>
#include <packeteer/error.h>

#include <packeteer/net/socket_address.h>
#include <packeteer/net/detail/cidr.h>


#define NETWORK_LIMIT 64

namespace packeteer {
namespace net {

/*****************************************************************************
 * Helper functions
 **/
namespace {

inline uint32_t make_mask32(size_t mask_size)
{
  uint32_t mask = 0;
  for (size_t i = 0 ; i < mask_size ; ++i) {
    mask |= 1 << (31 - i);
  }
  return mask;
}


inline void make_mask128(sockaddr_in6 & mask, size_t mask_size)
{
  ::memset(mask.sin6_addr.s6_addr, 0, 16);
  for (ssize_t i = mask_size, j = 0 ; i > 0 ; i -= 8, ++j) {
    mask.sin6_addr.s6_addr[j] = i >= 8
      ? 0xff
      : 0xff << (8 - i);
  }
}


inline uint64_t pow2(size_t exp)
{
  if (exp > NETWORK_LIMIT) {
    throw exception(ERR_INVALID_VALUE, "Network is larger than supported.");
  }
  static const uint64_t powers[NETWORK_LIMIT] = {
    1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768,
    65536, 131072, 262144, 524288, 1048576, 2097152, 4194304, 8388608, 16777216,
    33554432, 67108864, 134217728, 268435456, 536870912, 1073741824, 2147483648,
    4294967296, 8589934592, 17179869184, 34359738368, 68719476736, 137438953472,
    274877906944, 549755813888, 1099511627776, 2199023255552, 4398046511104,
    8796093022208, 17592186044416, 35184372088832, 70368744177664,
    140737488355328, 281474976710656, 562949953421312, 1125899906842624,
    2251799813685248, 4503599627370496, 9007199254740992, 18014398509481984,
    36028797018963968, 72057594037927936, 144115188075855872, 288230376151711744,
    576460752303423488, 1152921504606846976, 2305843009213693952,
    4611686018427387904, 9223372036854775808ULL,
  };
  return powers[exp];
}



} // anonymous namespace


/*****************************************************************************
 * Implementation
 **/
struct network::network_impl
{
  std::string               m_netspec;  // Keep original
  socket_address            m_network;  // Parsed network address
  size_t                    m_mask_size;
  sa_family_t               m_family;
  std::set<socket_address>  m_allocated;

  network_impl(std::string const & netspec)
    : m_netspec(netspec)
    , m_network("0.0.0.0") // Fake for now, see below
    , m_mask_size(0)
    , m_family(AF_UNSPEC)
  {
    // We couldn't parse the full netspec in the initializer, because it
    // contains a netmask, and socket_address doesn't like that. But we can
    // easily parse it now. XXX This is a little wasteful, parsing two
    // netspecs, but not as bad as convoluting the cidr API even more.
    detail::parse_result_t result(m_network.data);
    error_t err = detail::parse_extended_cidr(netspec, false, result);
    if (ERR_SUCCESS != err) {
      throw exception(err, "Could not parse CIDR specification.");
    }
    m_mask_size = result.mask;
    m_family = result.proto;
  }


  inline uint64_t get_max() const
  {
    // Hard limit the maximum
    size_t limit = NETWORK_LIMIT;
    if (AF_INET == m_family) {
      limit = 32; // Probably a lower limit.
    }

    ssize_t max_exp = limit - m_mask_size;
    if (max_exp < 0) {
      throw exception(ERR_UNEXPECTED, "It seems the network limit is smaller than the network mask.");
    }

    return pow2(max_exp) - 2;
  }
};


/*****************************************************************************
 * Member functions
 **/
network::network(std::string const & netspec)
  : m_impl(new network_impl(netspec))
{
}



network::~network()
{
  delete m_impl;
}



void
network::reset(std::string const & netspec)
{
  delete m_impl;
  m_impl = new network_impl(netspec);
}



bool
network::verify_netspec(std::string const & netspec)
{
  detail::address_type dummy_addr;
  detail::parse_result_t result(dummy_addr);
  error_t err = detail::parse_extended_cidr(netspec, false, result);
  return (err == ERR_SUCCESS);
}



size_t
network::mask_size() const
{
  return m_impl->m_mask_size;
}



sa_family_t
network::family() const
{
  return m_impl->m_family;
}



bool
network::in_network(socket_address const & address) const
{
  // Thanks to bitmasking magic, the address is in the network if it's masked
  // version is the same as the network address.
  return network_address() == make_masked(address);
}



socket_address
network::network_address() const
{
  return make_masked(m_impl->m_network);
}



socket_address
network::broadcast_address() const
{
  socket_address addr(m_impl->m_network);

  if (AF_INET == m_impl->m_family) {
    // IPv4 addresses are pretty easy to handle. They're 32 bits long, so all
    // we need is a 32 bit mask for them.
    uint32_t mask = make_mask32(m_impl->m_mask_size);

    // Apply the mask
    uint32_t ip = ntohl(addr.data.sa_in.sin_addr.s_addr);
    ip |= ~mask;
    addr.data.sa_in.sin_addr.s_addr = htonl(ip);
    addr.data.sa_in.sin_port = htons(UINT16_MAX);
  }
  else {
    // IPv6 addresses are a bit more difficult, because there are no (portable)
    // 128 bit operations available.
    sockaddr_in6 mask;
    make_mask128(mask, m_impl->m_mask_size);

    // Apply the mask
    for (size_t i = 0 ; i < 16 ; ++i) {
      addr.data.sa_in6.sin6_addr.s6_addr[i] |= ~mask.sin6_addr.s6_addr[i];
    }
    addr.data.sa_in6.sin6_port = htons(UINT16_MAX);
  }

  return addr;

}



socket_address
network::reserve_address()
{
  // It's easy to calculate whether we've already allocated all addresses in
  // the network.
  uint64_t max = m_impl->get_max();
  size_t amount = m_impl->m_allocated.size();
  if (max <= amount) {
    // Too many already.
    throw exception(ERR_NUM_ITEMS, "Too many addresses already reserved.");
  }

  // The lowest allowed is the network address plus one.
  socket_address alloc = network_address();
  ++alloc;

  // We know that if we have no addresses, we can just return the lowest
  // allowed.
  if (0 == amount) {
    m_impl->m_allocated.insert(alloc);
    return alloc;
  }

  // Because we don't know how things are deallocated, we can't assume that
  // we just add one to the last allocated address and we're good. Instead,
  // we have to iterate (sad but true).
  auto end = m_impl->m_allocated.end();
  for (auto iter = m_impl->m_allocated.begin() ; iter != end ; ++iter) {
    if (!(*iter == alloc)) {
      m_impl->m_allocated.insert(alloc);
      return alloc;
    }
    ++alloc;
  }

  // If we've exhausted the allocated addresses, then alloc *should* be fine
  // Let's make doubly sure, though.
  if (alloc < broadcast_address()) {
    m_impl->m_allocated.insert(alloc);
    return alloc;
  }

  // Should never happen.
  throw exception(ERR_UNEXPECTED, "Should be an unreachable line.");
}



socket_address
network::reserve_address(std::string const & identifier)
{
  // std::cout << "id: " << identifier << std::endl;

  std::hash<std::string> hasher;
  size_t hash = hasher(identifier);
  // std::cout << "hash pre: " << hash << std::endl;

  // That the hash is large is great, but we will have less than 32 bits
  // available for addresses... so truncate this further.
  hash %= m_impl->get_max();
  // std::cout << "hash post: " << hash << std::endl;

  // The lowest allowed is the network address plus one.
  socket_address alloc = network_address();
  ++alloc;

  // Increment the allocated address as often as the hash value now.
  for (uint32_t i = 0 ; i < hash ; ++i, ++alloc);

  // The address we found may already be allocated. In this version of
  // reserve_address() we just give up, then.
  auto pos = m_impl->m_allocated.find(alloc);
  if (m_impl->m_allocated.end() == pos) {
    // Not yet allocated! Let's allocate and return it, then!
    m_impl->m_allocated.insert(alloc);
    return alloc;
  }

  // Well, it looks grim. There must have been a collision.
  throw exception(ERR_NUM_ITEMS, "Possible hash collision when allocating addresses.");
}



socket_address
network::reserve_address(void const * identifier, size_t const & length)
{
  if (nullptr == identifier || 0 == length) {
    throw exception(ERR_INVALID_VALUE, "No or zero length identifier specified.");
  }

  return reserve_address(std::string(static_cast<char const *>(identifier),
        length));
}



bool
network::reserve_address(socket_address const & addr)
{
  if (!in_network(addr)) {
    return false;
  }

  auto iter = m_impl->m_allocated.find(addr);
  if (m_impl->m_allocated.end() != iter) {
    // Already reserved
    return false;
  }

  m_impl->m_allocated.insert(addr);
  return true;
}



bool
network::release_address(socket_address const & addr)
{
  auto iter = m_impl->m_allocated.find(addr);
  if (m_impl->m_allocated.end() == iter) {
    return false;
  }

  m_impl->m_allocated.erase(iter);
  return true;
}



socket_address
network::make_masked(socket_address const & input) const
{
  socket_address addr(input);

  if (AF_INET == m_impl->m_family) {
    // IPv4 addresses are pretty easy to handle. They're 32 bits long, so all
    // we need is a 32 bit mask for them.
    uint32_t mask = make_mask32(m_impl->m_mask_size);

    // Apply the mask
    uint32_t ip = ntohl(addr.data.sa_in.sin_addr.s_addr);
    ip &= mask;
    addr.data.sa_in.sin_addr.s_addr = htonl(ip);
    addr.data.sa_in.sin_port = 0;
  }
  else {
    // IPv6 addresses are a bit more difficult, because there are no (portable)
    // 128 bit operations available.
    sockaddr_in6 mask;
    make_mask128(mask, m_impl->m_mask_size);

    // Apply the mask
    for (size_t i = 0 ; i < 16 ; ++i) {
      addr.data.sa_in6.sin6_addr.s6_addr[i] &= mask.sin6_addr.s6_addr[i];
    }
    addr.data.sa_in6.sin6_port = 0;
  }

  return addr;
}



uint64_t
network::max_size() const
{
  return m_impl->get_max();
}



bool
network::is_equal_to(network const & other) const
{
  return (m_impl->m_mask_size == other.m_impl->m_mask_size
      && m_impl->m_family == other.m_impl->m_family
      && m_impl->m_network == other.m_impl->m_network);
}

bool
network::is_less_than(network const & other) const
{
  // See socket_address logic
  if (m_impl->m_family != other.m_impl->m_family) {
    return false;
  }

  // If one network is smaller than the other, it makes sense to return
  // true.
  if (m_impl->m_network < other.m_impl->m_network) {
    return true;
  }

  // Compare masks
  return (m_impl->m_mask_size < other.m_impl->m_mask_size);
}




/*****************************************************************************
 * Friend functions
 **/
std::ostream &
operator<<(std::ostream & os, network const & addr)
{
  os << addr.m_impl->m_network.cidr_str() << "/" << addr.m_impl->m_mask_size;
  return os;
}


}} // namespace packeteer::net
