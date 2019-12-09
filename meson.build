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
  cpp_args = posix_common_args + [
    '-Wabi'
  ]
  cpp_lib_args = ['-DPACKETEER_IS_BUILDING=1']
elif compiler_id == 'gcc'
  cpp_args = posix_common_args + [
    '-Wstrict-null-sentinel',
  ]
  cpp_lib_args = ['-DPACKETEER_IS_BUILDING=1']
elif compiler_id == 'msvc'
  cpp_args = [
    '/W4', '/wd4250', '/wd4251', '/wd4275',
  ]
  cpp_lib_args = ['/DPACKETEER_IS_BUILDING=1']
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
if compiler.has_header('experimental/propagate_const')
  conf_data.set('PACKETEER_HAVE_EXPERIMENTAL_PROPAGATE_CONST', true)
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


### Set values from options

cache_line_size = get_option('cache_line_size')
message('Set cache line size option to: @0@'.format(cache_line_size))
conf_data.set('PACKETEER_CACHE_LINE_SIZE', cache_line_size)

configure_file(
  input: 'build-config.h.in',
  output: 'build-config.h',
  configuration: conf_data)



##############################################################################
# Dependencies

thread = dependency('threads', required: true)

##############################################################################
# Library

includes = include_directories(
  'include',
)

libincludes = include_directories(
  'lib',
)


install_headers(
  'include' / 'packeteer.h',
  'include' / 'packeteer' / 'version.h',
  'include' / 'packeteer' / 'error.h',
  'include' / 'packeteer' / 'types.h',
  'include' / 'packeteer' / 'scheduler.h',
  'include' / 'packeteer' / 'scheduler' / 'types.h',
  'include' / 'packeteer' / 'scheduler' / 'callback.h',
  'include' / 'packeteer' / 'scheduler' / 'events.h',
  'include' / 'packeteer' / 'handle.h',
  'include' / 'packeteer' / 'connector.h',
  'include' / 'packeteer' / 'connector' / 'types.h',
  'include' / 'packeteer' / 'connector' / 'interface.h',
  'include' / 'packeteer' / 'connector' / 'peer_address.h',

  subdir: 'packeteer',
)

install_headers(
  'include' / 'packeteer' / 'net' / 'socket_address.h',
  'include' / 'packeteer' / 'net' / 'network.h',

  subdir: 'packeteer' / 'net',
)

install_headers(
  'include' / 'packeteer' / 'util' / 'operators.h',
  'include' / 'packeteer' / 'util' / 'hash.h',
  'include' / 'packeteer' / 'util' / 'url.h',

  subdir: 'packeteer' / 'util',
)


libsrc = [
  'lib' / 'version.cpp',
  'lib' / 'error.cpp',
  'lib' / 'packeteer.cpp',
  'lib' / 'registry.cpp',
  'lib' / 'scheduler.cpp',
  'lib' / 'connector.cpp',
  'lib' / 'connector' / 'peer_address.cpp',
  'lib' / 'scheduler' / 'worker.cpp',
  'lib' / 'scheduler' / 'scheduler_impl.cpp',
  'lib' / 'net' / 'socket_address.cpp',
  'lib' / 'net' / 'network.cpp',
  'lib' / 'net' / 'cidr.cpp',
  'lib' / 'thread' / 'tasklet.cpp',
  'lib' / 'util' / 'url.cpp',
  'lib' / 'util' / 'string.cpp',
]


if have_epoll_create
  libsrc += ['lib' / 'scheduler' / 'io' / 'epoll.cpp']
endif

if have_select
  libsrc += ['lib' / 'scheduler' / 'io' / 'select.cpp']
endif

if have_poll
  libsrc +=  ['lib' / 'scheduler' / 'io' / 'poll.cpp']
endif

if have_kqueue
  libsrc +=  ['lib' / 'scheduler' / 'io' / 'kqueue.cpp']
endif

posixsrc = [
  'lib' / 'posix' / 'handle.cpp',
  'lib' / 'connector' / 'posix' / 'interface.cpp',
  'lib' / 'connector' / 'posix' / 'socket.cpp',
  'lib' / 'connector' / 'posix' / 'pipe.cpp',
  'lib' / 'connector' / 'posix' / 'tcp.cpp',
  'lib' / 'connector' / 'posix' / 'udp.cpp',
  'lib' / 'connector' / 'posix' / 'anon.cpp',
  'lib' / 'connector' / 'posix' / 'local.cpp',
]

winsrc = [
  'lib' / 'win32' / 'handle.cpp',
# TODO move to connector/win32
  'lib' / 'win32' / 'detail' / 'connector.cpp',
  'lib' / 'win32' / 'detail' / 'connector_socket.cpp',
  'lib' / 'win32' / 'detail' / 'connector_pipe.cpp',
  'lib' / 'win32' / 'detail' / 'connector_tcp.cpp',
  'lib' / 'win32' / 'detail' / 'connector_udp.cpp',
  'lib' / 'win32' / 'detail' / 'connector_anon.cpp',
]


if host_type == 'posix' or host_type == 'android'
  libsrc += posixsrc
elif host_type == 'win32'
  libsrc += winsrc
endif

lib = library('packeteer', libsrc,
    include_directories: [includes, libincludes],
    dependencies: [thread],
    link_args: link_args,
    cpp_args: cpp_lib_args,
    version: LIB_VERSION,
    soversion: ABI_VERSION,
    install: true)

packeteer_dep = declare_dependency(
    include_directories: [includes],
    sources: [lib],
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

  # We're building two tests:
  # - public_tests include *only* public headers
  # - private_tests include private headers

  public_test_src = [
    'test' / 'public' / 'test_registry.cpp',
    'test' / 'public' / 'test_error.cpp',
    'test' / 'public' / 'test_handle.cpp',
    'test' / 'public' / 'test_util_url.cpp',
    'test' / 'public' / 'test_net_network.cpp',
    'test' / 'public' / 'test_net_socket_address.cpp',
    'test' / 'public' / 'test_scheduler_callback.cpp',
    'test' / 'public' / 'test_scheduler.cpp',
    'test' / 'public' / 'test_connector_peer_address.cpp',
    'test' / 'public' / 'test_connector.cpp',
    'test' / 'runner.cpp',
  ]

  private_test_src = [
    'test' / 'private' / 'test_concurrent_queue.cpp',
    'test' / 'private' / 'test_scheduler_containers.cpp',
    'test' / 'private' / 'test_net_cidr.cpp',
    'test' / 'private' / 'test_thread_binder.cpp',
    'test' / 'private' / 'test_thread_tasklet.cpp',
    'test' / 'runner.cpp',
  ]

  public_tests = executable('public_tests', public_test_src,
      include_directories: [includes],  # public headers only!
      dependencies: [
        gtest.get_variable('gtest_dep')
      ],
      link_with: lib,
      cpp_args: test_args,
      link_args: link_args,
  )
  test('public_tests', public_tests)

  # Due to symbol visibility, private tests won't link for non-debug builds
  if bt in ['debug', 'debugoptimized']
    private_tests = executable('private_tests', private_test_src,
        include_directories: [includes, libincludes],  # public and private headers
        dependencies: [
          gtest.get_variable('gtest_dep')
        ],
        link_with: lib,
        cpp_args: test_args,
        link_args: link_args,
    )
    test('private_tests', private_tests)
  endif

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
