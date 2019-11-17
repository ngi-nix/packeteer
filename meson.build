##############################################################################
# Project
project('packeteer', 'cpp',
  version: '0.1.0',
  license: 'GPLv3',
  default_options: [
    'cpp_std=c++17',
    'default_library=both',
  ])


##############################################################################
# Versioning, project and libraries

# There are so many versioning schemes, each of which is incompatible with
# others. We'll manage best by keeping things simple:
#
# - The package version follows semver
# - The library version is the package version
# - The ABI version, i.e. the compatibility we expect, is the major
#   component of the package
_splitver = meson.project_version().split('.')
PACKAGE_MAJOR = _splitver[0]
PACKAGE_MINOR = _splitver[1]
PACKAGE_PATCH = _splitver[2]

ABI_VERSION = PACKAGE_MAJOR
LIB_VERSION = meson.project_version()
PACKAGE_VERSION = meson.project_version()

##############################################################################
# Configuration

conf_data = configuration_data()
compiler = meson.get_compiler('cpp')

host_type = ''
if host_machine.system() in [ 'cygwin', 'darwin', 'dragonfly', 'freebsd', 'gnu', 'linux', 'netbsd' ]
  host_type = 'posix'
elif host_machine.system() == 'windows'
  host_type = 'win32'
elif host_machine.system().startswith('android')
  host_type = 'android'
endif

### Compiler flags
compiler_id = compiler.get_id()

posix_common_args = [
  '-Wall', '-Wextra', '-Wpedantic', '-Wshadow', '-Wstrict-aliasing',
  '-Wstrict-overflow=5', '-Wcast-align', '-Wmissing-declarations',
  '-Wpointer-arith', '-Wcast-qual', '-Wold-style-cast', '-Wabi',
]
if compiler_id == 'clang'
  cpp_args = posix_common_args + []
elif compiler_id == 'gcc'
  cpp_args = posix_common_args + [
    '-Wstrict-null-sentinel',
  ]
elif compiler_id == 'msvc'
  cpp_args = [
    '/W4', '/wd4250', '/wd4251', '/wd4275',
  ]
endif

bt = get_option('buildtype')
if bt in ['debug', 'debugoptimized']
  if compiler_id == 'clang'
    cpp_args += ['-ggdb', '-DDEBUG']
  elif compiler_id == 'gcc'
    cpp_args += ['-g3', '-DDEBUG']
  elif compiler_id == 'msvc'
    # HAHAHA, VC tries to default to non-permissive mode via /permissive-, but
    # Windows SDK headers are non-conforming.
    cpp_args += ['/Zi', '/FS', '/permissive']
  endif
else
  posix_release_args = [
    '-DNDEBUG',
    '-fvisibility=hidden', '-fvisibility-inlines-hidden',
  ]
  if compiler_id == 'clang'
    cpp_args += posix_release_args
  elif compiler_id == 'gcc'
    cpp_args += posix_release_args
  elif compiler_id == 'msvc'
    cpp_args += ['/Oi']
  endif
endif

link_args = []
if host_type == 'android'
  # Only posix compilers supported (?)
  cpp_args += [
    '-DANDROID_STL=c++_shared',
    '-fexceptions', '-frtti',
  ]
  link_args = [
    '-lstdc++', '-lm',
  ]
endif

add_project_arguments(cpp_args, language: 'cpp')


### Version and package information
conf_data.set_quoted('PACKETEER_PACKAGE_MAJOR', PACKAGE_MAJOR)
conf_data.set_quoted('PACKETEER_PACKAGE_MINOR', PACKAGE_MINOR)
conf_data.set_quoted('PACKETEER_PACKAGE_PATCH', PACKAGE_PATCH)
conf_data.set_quoted('PACKETEER_PACKAGE_VERSION', PACKAGE_VERSION)
conf_data.set_quoted('PACKETEER_ABI_VERSION', ABI_VERSION)
conf_data.set_quoted('PACKETEER_LIB_VERSION', LIB_VERSION)

conf_data.set_quoted('PACKETEER_PACKAGE_NAME', meson.project_name())
conf_data.set_quoted('PACKETEER_PACKAGE_URL', 'https://gitlab.com/jfinkhaeuser/packeteer')

### Host platform details
conf_data.set('PACKETEER_BIGENDIAN', host_machine.endian() == 'big')

### Headers
if compiler.has_header('unistd.h')
  conf_data.set('PACKETEER_HAVE_UNISTD_H', true)
endif
if compiler.has_header('sys' / 'select.h')
  conf_data.set('PACKETEER_HAVE_SYS_SELECT_H', true)
endif
if compiler.has_header('sys' / 'time.h')
  conf_data.set('PACKETEER_HAVE_SYS_TIME_H', true)
endif
if compiler.has_header('sys' / 'epoll.h')
  conf_data.set('PACKETEER_HAVE_SYS_EPOLL_H', true)
endif
if compiler.has_header('time.h')
  conf_data.set('PACKETEER_HAVE_TIME_H', true)
endif
if compiler.has_header('poll.h')
  conf_data.set('PACKETEER_HAVE_POLL_H', true)
endif
if compiler.has_header('arpa' / 'inet.h')
  conf_data.set('PACKETEER_HAVE_ARPA_INET_H', true)
endif
if compiler.has_header('netinet' / 'in.h')
  conf_data.set('PACKETEER_HAVE_NETINET_IN_H', true)
endif
if compiler.has_header('linux' / 'un.h')
  conf_data.set('PACKETEER_HAVE_LINUX_UN_H', true)
endif
if compiler.has_header('sys' / 'un.h')
  conf_data.set('PACKETEER_HAVE_SYS_UN_H', true)
endif
if compiler.has_header('sys' / 'socket.h')
  conf_data.set('PACKETEER_HAVE_SYS_SOCKET_H', true)
endif
if compiler.has_header('winsock2.h')
  conf_data.set('PACKETEER_HAVE_WINSOCK2_H', true)
endif
if compiler.has_header('ws2tcpip.h')
  conf_data.set('PACKETEER_HAVE_WS2TCPIP_H', true)
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

thread = dependency('threads', required: true)

configure_file(
  input: 'packeteer-config.h.in',
  output: 'packeteer-config.h',
  configuration: conf_data)



##############################################################################
# Library

libsrc = [
  'packeteer' / 'version.cpp',
  'packeteer' / 'error.cpp',
  'packeteer' / 'packeteer.cpp',
  'packeteer' / 'scheduler.cpp',
  'packeteer' / 'connector.cpp',
  'packeteer' / 'peer_address.cpp',
  'packeteer' / 'detail' / 'worker.cpp',
  'packeteer' / 'detail' / 'scheduler_impl.cpp',
  'packeteer' / 'net' / 'socket_address.cpp',
  'packeteer' / 'net' / 'network.cpp',
  'packeteer' / 'net' / 'detail' / 'cidr.cpp',
  'packeteer' / 'thread' / 'tasklet.cpp',
  'packeteer' / 'util' / 'url.cpp',
]

if have_epoll_create
  libsrc += ['packeteer' / 'detail' / 'io_epoll.cpp']
endif

if have_select
  libsrc += ['packeteer' / 'detail' / 'io_select.cpp']
endif

if have_poll
  libsrc +=  ['packeteer' / 'detail' / 'io_poll.cpp']
endif

if have_kqueue
  libsrc +=  ['packeteer' / 'detail' / 'io_kqueue.cpp']
endif

posixsrc = [
  'packeteer' / 'posix' / 'handle.cpp',
  'packeteer' / 'posix' / 'detail' / 'connector.cpp',
  'packeteer' / 'posix' / 'detail' / 'connector_socket.cpp',
  'packeteer' / 'posix' / 'detail' / 'connector_pipe.cpp',
  'packeteer' / 'posix' / 'detail' / 'connector_tcp.cpp',
  'packeteer' / 'posix' / 'detail' / 'connector_udp.cpp',
  'packeteer' / 'posix' / 'detail' / 'connector_anon.cpp',
  'packeteer' / 'posix' / 'detail' / 'connector_local.cpp',
]

winsrc = [
  'packeteer' / 'win32' / 'handle.cpp',
  'packeteer' / 'win32' / 'detail' / 'connector.cpp',
  'packeteer' / 'win32' / 'detail' / 'connector_socket.cpp',
  'packeteer' / 'win32' / 'detail' / 'connector_pipe.cpp',
  'packeteer' / 'win32' / 'detail' / 'connector_tcp.cpp',
  'packeteer' / 'win32' / 'detail' / 'connector_udp.cpp',
  'packeteer' / 'win32' / 'detail' / 'connector_anon.cpp',
]


if host_type == 'posix' or host_type == 'android'
  libsrc += posixsrc
elif host_type == 'win32'
  libsrc += winsrc
endif

lib = library('packeteer', libsrc,
    include_directories: [],
    dependencies: [thread],
    link_args: link_args,
    version: LIB_VERSION,
    soversion: ABI_VERSION,
    install: true)

packeteer_dep = declare_dependency(
    sources: [lib], # FIXME headers
    version: LIB_VERSION,
)

##############################################################################
# Meson Wrap Dependencies
gtest = subproject('gtest')

##############################################################################
# Tests

# Tests in a subproject are complicicated. You need all the compile and
# link flags from the enclosing project, etc.
if not meson.is_subproject()

  # See https://github.com/google/googletest/issues/813
  test_args = []
  if host_machine.system() == 'cygwin'
    test_args += ['-D_POSIX_C_SOURCE=200809L']
  endif

  testsrc = [
    'test' / 'test_error.cpp',
    'test' / 'test_cidr.cpp',
    'test' / 'test_handle.cpp',
    'test' / 'test_url.cpp',
    'test' / 'test_socket_address.cpp',
    'test' / 'test_network.cpp',
    'test' / 'test_callback.cpp',
    'test' / 'test_concurrent_queue.cpp',
    'test' / 'test_scheduler_containers.cpp',
    'test' / 'test_scheduler.cpp',
    'test' / 'test_peer_address.cpp',
    'test' / 'test_connector.cpp',
    'test' / 'test_binder.cpp',
    'test' / 'test_tasklet.cpp',
    'test' / 'testsuite.cpp',
  ]

  testsuite = executable('testsuite', testsrc,
      include_directories: [],
      dependencies: [
        gtest.get_variable('gtest_dep')
      ],
      link_with: lib,
      cpp_args: cpp_args + test_args,
      link_args: link_args,
  )
  test('testsuite', testsuite)

endif

##############################################################################
# Linter, etc.
cppcheck = find_program('cppcheck', required: false)

if cppcheck.found()
  run_target('cppcheck', command: [
    cppcheck.path(),
    '--cppcheck-build-dir=@0@/cppcheck'.format(meson.current_build_dir()),
    '--enable=all', '-v',
    '@0@/packeteer'.format(meson.current_source_dir()),
  ])
endif
