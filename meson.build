##############################################################################
# Project
project('packeteer', 'cpp',
  version: '0.1',
  default_options: [
    'cpp_std=c++17',
  ])

##############################################################################
# Configuration

conf_data = configuration_data()
compiler = meson.get_compiler('cpp')

### Compiler flags
compiler_id = compiler.get_id()

if compiler_id == 'clang'
  extra_args = ['-Wall', '-Wextra', '-Wabi', '-Wold-style-cast']
elif compiler_id == 'gcc'
  extra_args = ['-Wall', '-Wextra', '-Wold-style-cast', '-Wstrict-null-sentinel']
endif


### Version and package information
ver = meson.project_version()
splitver = ver.split('.')
conf_data.set('PACKETEER_PACKAGE_MAJOR', splitver[0])
conf_data.set('PACKETEER_PACKAGE_MINOR', splitver[1])

conf_data.set('PACKETEER_PACKAGE_NAME', meson.project_name())
conf_data.set('PACKETEER_PACKAGE_URL', 'https://github.com/jfinkhaeuser/packeteer')

### Host platform details
conf_data.set('PACKETEER_BIGENDIAN', host_machine.endian() == 'big')

### Headers
if compiler.has_header('sys/select.h')
  conf_data.set('PACKETEER_HAVE_SYS_SELECT_H', true)
endif
if compiler.has_header('sys/time.h')
  conf_data.set('PACKETEER_HAVE_SYS_TIME_H', true)
endif
if compiler.has_header('sys/epoll.h')
  conf_data.set('PACKETEER_HAVE_SYS_EPOLL_H', true)
endif
if compiler.has_header('time.h')
  conf_data.set('PACKETEER_HAVE_TIME_H', true)
endif
if compiler.has_header('poll.h')
  conf_data.set('PACKETEER_HAVE_POLL_H', true)
endif
if compiler.has_header('arpa/inet.h')
  conf_data.set('PACKETEER_HAVE_ARPA_INET_H', true)
endif
if compiler.has_header('netinet/in.h')
  conf_data.set('PACKETEER_HAVE_NETINET_IN_H', true)
endif
if compiler.has_header('linux/un.h')
  conf_data.set('PACKETEER_HAVE_LINUX_UN_H', true)
endif


### Types

compiler.sizeof('int32_t', prefix: '#include <stdint.h>')
compiler.sizeof('uint32_t', prefix: '#include <stdint.h>')
compiler.sizeof('int64_t', prefix: '#include <stdint.h>')
compiler.sizeof('uint64_t', prefix: '#include <stdint.h>')
compiler.sizeof('size_t', prefix: '#include <stdint.h>')
compiler.sizeof('ssize_t', prefix: '#include <stdint.h>')


have_epoll_create = compiler.compiles('''
#include <sys/epoll.h>

int main(int, char**)
{
  int foo = epoll_create1(EPOLL_CLOEXEC);
}
''', name: 'epoll_create1()')
if have_epoll_create
  conf_data.set('PACKETEER_HAVE_EPOLL_CREATE1', true)
endif


have_select = compiler.compiles('''
#include <sys/select.h>
#include <string.h>

int main(int, char**)
{
  int ret = select(0, NULL, NULL, NULL, NULL);
}
''', name: 'select()')
if have_select
  conf_data.set('PACKETEER_HAVE_SELECT', true)
endif


have_pselect = compiler.compiles('''
#include <sys/select.h>
#include <string.h>

int main(int, char**)
{
  int ret = pselect(0, NULL, NULL, NULL, NULL, NULL);
}
''', name: 'pselect()')
if have_pselect
  conf_data.set('PACKETEER_HAVE_PSELECT', true)
endif


have_poll = compiler.compiles('''
#include <poll.h>
#include <string.h>

int main(int, char**)
{
  int foo = poll(NULL, 0, 0);
}
''', name: 'poll()')
if have_poll
  conf_data.set('PACKETEER_HAVE_POLL', true)
endif


have_ppoll = compiler.compiles('''
#include <poll.h>
#include <string.h>

int main(int, char**)
{
  int foo = ppoll(NULL, 0, 0, NULL);
}
''', name: 'ppoll()')
if have_ppoll
  conf_data.set('PACKETEER_HAVE_PPOLL', true)
endif


have_pollrdhup = compiler.compiles('''
#include <poll.h>
#include <string.h>

int main(int, char**)
{
  int foo = POLLRDHUP;
}
''', name: 'pollrdhup()')
if have_pollrdhup
  conf_data.set('PACKETEER_HAVE_POLLRDHUP', true)
endif


have_pollhup = compiler.compiles('''
#include <poll.h>
#include <string.h>

int main(int, char**)
{
  int foo = POLLHUP;
}
''', name: 'pollhup()')
if have_pollhup
  conf_data.set('PACKETEER_HAVE_POLLHUP', true)
endif


have_kqueue = compiler.compiles('''
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

int main(int, char **)
{
  int foo = kqueue();
}
''', name: 'kqueue()')
if have_kqueue
  conf_data.set('PACKETEER_HAVE_KQUEUE', true)
endif


have_mknod = compiler.compiles('''
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

int main(int, char**)
{
  mknod("foo", S_IFIFO, S_IRUSR);
}
''', name: 'mknod()')
if have_mknod
  conf_data.set('PACKETEER_HAVE_MKNOD', true)
endif


have_mkfifo = compiler.compiles('''
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

int main(int, char**)
{
  mkfifo("foo", S_IRUSR);
}
''', name: 'mkfifo()')
if have_mkfifo
  conf_data.set('PACKETEER_HAVE_MKFIFO', true)
endif


### Dependencies

thread_dep = dependency('threads', required: true)

cppunit_dep = dependency('cppunit', required: false)

configure_file(
  input: 'packeteer-config.h.in',
  output: 'packeteer-config.h',
  configuration: conf_data)



##############################################################################
# Sources

libsrc = [
  'packeteer/version.cpp',
  'packeteer/error.cpp',
  'packeteer/packeteer.cpp',
  'packeteer/scheduler.cpp',
  'packeteer/connector.cpp',
  'packeteer/peer_address.cpp',
  'packeteer/detail/worker.cpp',
  'packeteer/detail/scheduler_impl.cpp',
  'packeteer/detail/connector.cpp',
  'packeteer/net/socket_address.cpp',
  'packeteer/net/network.cpp',
  'packeteer/net/detail/cidr.cpp',
  'packeteer/thread/tasklet.cpp',
  'packeteer/util/url.cpp',
]

if have_epoll_create
  libsrc += ['packeteer/detail/io_epoll.cpp']
endif

if have_select
  libsrc += ['packeteer/detail/io_select.cpp']
endif

if have_poll
  libsrc +=  ['packeteer/detail/io_poll.cpp']
endif

if have_kqueue
  libsrc +=  ['packeteer/detail/io_kqueue.cpp']
endif

posixsrc = [
  'packeteer/posix/handle.cpp',
  'packeteer/posix/detail/connector_socket.cpp',
  'packeteer/posix/detail/connector_pipe.cpp',
  'packeteer/posix/detail/connector_tcp.cpp',
  'packeteer/posix/detail/connector_udp.cpp',
  'packeteer/posix/detail/connector_anon.cpp',
  'packeteer/posix/detail/connector_local.cpp',
]

winsrc = []

if host_machine.system() in [ 'cygwin', 'darwin', 'dragonfly', 'freebsd', 'gnu', 'linux', 'netbsd' ]
  libsrc += posixsrc
elif host_machine.system() == 'windows'
  libsrc + winsrc
endif

lib = both_libraries('packeteer', libsrc,
    include_directories: [],
    dependencies: [thread_dep],
    cpp_args: extra_args,
    install: true)

# TODO soversion, version, darwin_versions

if cppunit_dep.found()

  testsrc = [
    'test/test_error.cpp',
    'test/test_cidr.cpp',
    'test/test_handle.cpp',
    'test/test_url.cpp',
    'test/test_socket_address.cpp',
    'test/test_network.cpp',
    'test/test_callback.cpp',
    'test/test_concurrent_queue.cpp',
    'test/test_scheduler_impl.cpp',
    'test/test_scheduler.cpp',
    'test/test_peer_address.cpp',
    'test/test_connector.cpp',
    'test/test_binder.cpp',
    'test/test_tasklet.cpp',
    'test/testsuite.cpp',
  ]

  testsuite = executable('testsuite', testsrc,
      include_directories: [],
      dependencies: [cppunit_dep],
      cpp_args: extra_args,
      link_with: lib.get_static_lib())
  test('testsuite', testsuite)

endif
