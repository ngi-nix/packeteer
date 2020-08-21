/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2020 Jens Finkhaeuser.
 *
 * This software is licensed under the terms of the GNU GPLv3 for personal,
 * educational and non-profit use. For all other uses, alternative license
 * options are available. Please contact the copyright holder for additional
 * information, stating your intended usage.
 *
 * You can find the full text of the GPLv3 in the COPYING file in this code
 * distribution.
 *
 * This software is distributed on an "AS IS" BASIS, WITHOUT ANY WARRANTY
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.
 **/
#include <build-config.h>

#if defined(PACKETEER_HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if defined(PACKETEER_HAVE_FCNTL_H)
#include <fcntl.h>
#endif

#if defined(PACKETEER_HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif

#if defined(PACKETEER_HAVE_SYS_STAT_H)
#include <sys/stat.h>
#endif

#if defined(PACKETEER_HAVE_SYS_SOCKET_H)
#include <sys/socket.h>
#endif

#if defined(PACKETEER_HAVE_SYS_IOCTL_H)
#include <sys/ioctl.h>
#endif

#if defined(PACKETEER_HAVE_NET_IF_H)
#include <net/if.h>
#endif

#if defined(PACKETEER_HAVE_LINUX_IF_TUN_H)
#include <linux/if_tun.h>
#endif

#if defined(PACKETEER_HAVE_LINUX_SOCKIOS_H)
#include <linux/sockios.h>
#endif

#include <string>

#include <liberate/net/url.h>
#include <liberate/string/util.h>

#include <packeteer/registry.h>

#include "../lib/connector/posix/fd.h"
#include "../lib/connector/posix/common.h"
#include "../lib/connector/util.h"
#include "../lib/macros.h"



namespace packeteer::ext {

namespace {

/**
 * Device type
 */
enum device_type : uint8_t
{
  DEVICE_TUN = 0,
  DEVICE_TAP = 1,
};


/**
 * TUN/TAP creation options.
 */
struct tuntap_options
{
  device_type type;
  std::string name;
  int         mtu;
  int         txqueuelen;
};


/**
 * TUN/TAP "device" representation, when created.
 */
struct tuntap : public tuntap_options
{
  int fd;
};





#if defined(PACKETEER_HAVE_LINUX_IF_TUN_H)

inline error_t
configure_tuntap(tuntap_options & dev)
{
  // Create socket; need this for setting values
  int sock = ::socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    ERRNO_LOG("Can't create socket for ioctl().");
    return ERR_ABORTED;
  }

  int err = 0;

  // Try setting/getting MTU
  {
    ifreq ifr = { 0 };
    ::strncpy(ifr.ifr_name, dev.name.c_str(), IFNAMSIZ);

    if (dev.mtu > 0) {
      ifr.ifr_mtu = dev.mtu;
      err = ::ioctl(sock, SIOCSIFMTU, &ifr);
      if (err < 0) {
        ERRNO_LOG("Cannot set MTU on interface.");
        close(sock);
        return ERR_ABORTED;
      }
    }
    err = ::ioctl(sock, SIOCGIFMTU, &ifr);
    if (err < 0) {
      ERRNO_LOG("Cannot get MTU on interface.");
      close(sock);
      return ERR_ABORTED;
    }
    dev.mtu = ifr.ifr_mtu;
  }

  // Try setting TX queue length
#if defined(SIOCSIFTXQLEN) && defined(SIOCGIFTXQLEN)
  {
    ifreq ifr = { 0 };
    ::strncpy(ifr.ifr_name, dev.name.c_str(), IFNAMSIZ);

    if (dev.txqueuelen > 0) {
      ifr.ifr_qlen = dev.txqueuelen;
      err = ::ioctl(sock, SIOCSIFTXQLEN, &ifr);
      if (err < 0) {
        ERRNO_LOG("Cannot set TX queue length.");
        close(sock);
        return ERR_ABORTED;
      }
    }
    err = ::ioctl(sock, SIOCGIFTXQLEN, &ifr);
    if (err < 0) {
      ERRNO_LOG("Cannot get TX queue length.");
      close(sock);
      return ERR_ABORTED;
    }
    dev.txqueuelen = ifr.ifr_qlen;
  }
#endif // SIOCSIFTXQLEN

  // Also bring the device up, while we're here.
  {
    ifreq ifr = { 0 };
    ::strncpy(ifr.ifr_name, dev.name.c_str(), IFNAMSIZ);

    ifr.ifr_flags |= IFF_UP;
    err = ::ioctl(sock, SIOCSIFFLAGS, &ifr);
    if (err < 0) {
      ERRNO_LOG("Cannot bring interface up!");
      close(sock);
      return ERR_ABORTED;
    }
  }

  // All good
  close(sock);
  return ERR_SUCCESS;
}



error_t
create_tuntap_device(tuntap & dev)
{
  // Open clone device. That's our file descriptor.
  int fd = ::open("/dev/net/tun", O_RDWR | O_NONBLOCK);
  if (fd < 0) {
    ERRNO_LOG("Failed to open clone device.");
    switch (errno) {
      case EACCES:
      case EFAULT:
      case EPERM:
        return ERR_ACCESS_VIOLATION;

      case EINVAL:
        return ERR_INVALID_VALUE;

      case EMFILE:
      case ENFILE:
        return ERR_NUM_FILES;

      case ENOENT:
      case ENOSPC:
      case EISDIR:
      case ELOOP:
      case EDQUOT:
      case EEXIST:
      case ENAMETOOLONG:
      case ENODEV:
      case ENXIO:
      case ENOTDIR:
        return ERR_FS_ERROR;

      case ENOMEM:
        return ERR_OUT_OF_MEMORY;

      default:
        return ERR_UNEXPECTED;
    }
  }

  // Need to create our own ifreq union because ifreq is too short to hold
  // sockaddr_storage (you're supposed to allocate enough memory and cast it
  // to an ifreq pointer).
  // XXX Not currently used; but when setting IP addresses directly, it'll be
  //     necessary.
  union my_ifreq {
    struct ifreq sys_ifreq;
    struct {
      char name_pad[IFNAMSIZ];
      struct sockaddr_storage addr;
    } padding;
  };

  // Set up TUN/TAP device.
  my_ifreq store;
  ::memset(&store, 0, sizeof(my_ifreq));
  ifreq * ifr = reinterpret_cast<ifreq *>(&store);
  ifr->ifr_flags = IFF_NO_PI | (DEVICE_TUN == dev.type ? IFF_TUN : IFF_TAP);

  if (!dev.name.empty()) {
    ::strncpy(ifr->ifr_name, dev.name.c_str(), IFNAMSIZ);
  }

  int err = ::ioctl(fd, TUNSETIFF, ifr);
  if (err < 0) {
    ERRNO_LOG("Cannot create TUN/TAP interface.");
    close(fd);
    if (errno == EPERM) {
      return ERR_ACCESS_VIOLATION;
    }
    return ERR_ABORTED;
  }

  // Remember device name.
  if (!ifr->ifr_name) {
    ELOG("Got TUN/TAP device, but no device name.");
    close(fd);
    return ERR_ABORTED;
  }
  std::string name = ifr->ifr_name;

  // Try configuring the device
  dev.name = name;
  auto e = configure_tuntap(dev);
  if (e != ERR_SUCCESS) {
    close(fd);
    return e;
  }

  // All good
  dev.fd = fd;

  return ERR_SUCCESS;
}

#endif // PACKETEER_HAVE_LINUX_IF_TUN_H


/**
 * TUN/TAP connector
 */
struct connector_tuntap : public ::packeteer::detail::connector_common
{
public:
  connector_tuntap(tuntap_options const & tuntap, connector_options const & options)
    : connector_common(options)
    , m_tuntap(tuntap)
    , m_fd(-1)
  {
  }

  virtual ~connector_tuntap() {};

  virtual error_t listen()
  {
    if (m_fd != -1) {
      return ERR_INITIALIZATION;
    }

    // Try to create a device
    tuntap dev{m_tuntap};
    auto err = create_tuntap_device(dev);
    if (err != ERR_SUCCESS) {
      return err;
    }

    // Ok, that went well. Store detected options and file descriptor.
    m_tuntap.name = dev.name;
    m_tuntap.mtu = dev.mtu;
    m_tuntap.txqueuelen = dev.txqueuelen;
    m_fd = dev.fd;

    DLOG("TUN/TAP device: " << (m_tuntap.type == DEVICE_TUN ? "TUN" : "TAP")
        << " " << m_tuntap.name << " mtu " << m_tuntap.mtu
        << " qlen " << m_tuntap.txqueuelen);

    return ERR_SUCCESS;
  }


  virtual bool listening() const
  {
    return m_fd != -1;
  }

  virtual error_t connect()
  {
    return listen();
  }

  virtual bool connected() const
  {
    return listening();
  }

  virtual connector_interface * accept(liberate::net::socket_address &)
  {
    return this;
  }

  virtual handle get_read_handle() const
  {
    return m_fd;
  }

  virtual handle get_write_handle() const
  {
    return m_fd;
  }

  virtual error_t close()
  {
    // Ignore errors - should be CLOEXEC anyway
    if (m_fd != -1) {
      ::close(m_fd);
      m_fd = -1;
    }
  }

  /***************************************************************************
   * Setting accessors
   **/
  virtual bool is_blocking() const
  {
    bool blocking = false;
    auto err = detail::get_blocking_mode(m_fd, blocking);
    if (ERR_SUCCESS != err) {
      throw new packeteer::exception(err, "Could not determine blocking mode of FD!");
    }
    return blocking;
  }

private:
  tuntap_options  m_tuntap;
  int             m_fd;
};



inline error_t
parse_tuntap_options(tuntap_options & opts, device_type type, liberate::net::url const & url)
{
  // Type is given from the outside, don't try to detect it.
  opts.type = type;

  // Try the name.
  if (url.path[0] != '/') {
    ELOG("Invalid path format.");
    return ERR_INVALID_VALUE;
  }
  opts.name = url.path.substr(1);

  // TODO more path normalization?

  // Grab MTU if we have any
  auto iter = url.query.find("mtu");
  if (iter != url.query.end()) {
    try {
      opts.mtu = std::stoi(iter->second);
    } catch (std::exception const & ex) {
      ELOG("Error reading MTU: " << ex.what());
      return ERR_INVALID_VALUE;
    }
  }
  else {
    opts.mtu = -1;
  }

  // Grab TX queue length if we have any
  iter = url.query.find("txqueuelen");
  if (iter != url.query.end()) {
    try {
      opts.txqueuelen = std::stoi(iter->second);
    } catch (std::exception const & ex) {
      ELOG("Error reading TX queue len: " << ex.what());
      return ERR_INVALID_VALUE;
    }
  }
  else {
    opts.txqueuelen = -1;
  }

  return ERR_SUCCESS;
}


packeteer::connector_interface *
tun_creator(liberate::net::url const & url,
    packeteer::connector_type const &,
    packeteer::connector_options const & options,
    packeteer::registry::connector_info const * info)
{
  tuntap_options opts;
  auto err = parse_tuntap_options(opts, DEVICE_TUN, url);
  if (err != ERR_SUCCESS) {
    return nullptr;
  }

  return new connector_tuntap(opts, options);
}



packeteer::connector_interface *
tap_creator(liberate::net::url const & url,
    packeteer::connector_type const &,
    packeteer::connector_options const & options,
    packeteer::registry::connector_info const * info)
{
  tuntap_options opts;
  auto err = parse_tuntap_options(opts, DEVICE_TAP, url);
  if (err != ERR_SUCCESS) {
    return nullptr;
  }

  return new connector_tuntap(opts, options);
}


} // anonymous namespace


PACKETEER_API
error_t
register_connector_tuntap(std::shared_ptr<packeteer::api> api,
    packeteer::connector_type register_as /* = CT_USER */)
{
#if !defined(PACKETEER_HAVE_LINUX_IF_TUN_H)
  return ERR_NOT_IMPLEMENTED;
#endif

  // Register TUN and TAP schemes
  packeteer::registry::connector_info tun_info{
    register_as,
    CO_DATAGRAM|CO_NON_BLOCKING,
    CO_STREAM|CO_DATAGRAM|CO_BLOCKING|CO_NON_BLOCKING,
    tun_creator,
  };
  api->reg().add_scheme("tun", tun_info);

  packeteer::registry::connector_info tap_info{
    register_as,
    CO_DATAGRAM|CO_NON_BLOCKING,
    CO_STREAM|CO_DATAGRAM|CO_BLOCKING|CO_NON_BLOCKING,
    tap_creator,
  };
  api->reg().add_scheme("tap", tap_info);

  return ERR_SUCCESS;
}


} // namespace packeteer::ext
