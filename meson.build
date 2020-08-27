##############################################################################
# Project
project('packeteer', 'cpp',
  version: '0.1.0',
  license: 'GPLv3',
  meson_version: '>=0.53.0',
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
summary('Host platform', host_type, section: 'Platform')

# For Windows, try to determine the SDK version.
winsdk_ver = -1
if compiler.has_header('ntverp.h')
  code = '''#include <iostream>
#include <ntverp.h>

int main()
{
  std::cout << VER_PRODUCTMAJORVERSION;
}
'''
  result = compiler.run(code, name: 'Windows SDK version check.')
  winsdk_ver = result.stdout().to_int()
  summary('Windows SDK major version', winsdk_ver, section: 'Platform')
endif


### Compiler flags
compiler_id = compiler.get_id()

cpp_lib_is_building = []
cpp_args = []

posix_common_args = [
  '-Wall', '-Wextra', '-Wpedantic', '-Wshadow', '-Wstrict-aliasing',
  '-Wstrict-overflow=5', '-Wcast-align',
  '-Wpointer-arith', '-Wcast-qual', '-Wold-style-cast',
]
if compiler_id == 'clang'
  cpp_args += posix_common_args + [
    '-Wabi',
  ]
  cpp_lib_is_building = ['-DPACKETEER_IS_BUILDING=1']
elif compiler_id == 'gcc'
  cpp_args += posix_common_args + [
    '-Wstrict-null-sentinel',
  ]
  cpp_lib_is_building = ['-DPACKETEER_IS_BUILDING=1']
elif compiler_id == 'msvc'
  cpp_args += [
    '/wd4250', '/wd4251', '/wd4275',
  ]
  cpp_lib_is_building = ['/DPACKETEER_IS_BUILDING=1']
endif

bt = get_option('buildtype')
if bt in ['debug']
  if compiler_id == 'clang'
    cpp_args += ['-ggdb', '-DDEBUG']
  elif compiler_id == 'gcc'
    cpp_args += ['-g3', '-DDEBUG']
  elif compiler_id == 'msvc'
    cpp_args += ['/FS', '/DDEBUG=1']
  endif
elif bt in ['debugoptimized']
  if compiler_id == 'clang'
    cpp_args += ['-ggdb', '-DNDEBUG']
  elif compiler_id == 'gcc'
    cpp_args += ['-g3', '-DNDEBUG']
  elif compiler_id == 'msvc'
    cpp_args += ['/FS', '/DNDEBUG=1']
  endif
else
  posix_release_args = [
    '-fvisibility=hidden', '-fvisibility-inlines-hidden',
    '-DNDEBUG',
  ]
  if compiler_id == 'clang'
    cpp_args += posix_release_args
  elif compiler_id == 'gcc'
    cpp_args += posix_release_args
  elif compiler_id == 'msvc'
    cpp_args += ['/Oi', '/DNDEBUG=1']
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


# Make things work with MSVC and Windows SDK <10
if compiler_id == 'msvc' and winsdk_ver < 10
  cpp_args += ['/permissive']
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
conf_data.set_quoted('PACKETEER_PACKAGE_URL', 'https://gitlab.com/interpeer/packeteer')

### Host platform details
conf_data.set('PACKETEER_BIGENDIAN', host_machine.endian() == 'big')

### Headers
conf_data.set('PACKETEER_HAVE_UNISTD_H',
  compiler.has_header('unistd.h'))
conf_data.set('PACKETEER_HAVE_TIME_H',
  compiler.has_header('time.h'))
conf_data.set('PACKETEER_HAVE_POLL_H',
  compiler.has_header('poll.h'))
conf_data.set('PACKETEER_HAVE_FCNTL_H',
  compiler.has_header('fcntl.h'))

conf_data.set('PACKETEER_HAVE_SYS_TYPES_H',
  compiler.has_header('sys' / 'types.h'))
conf_data.set('PACKETEER_HAVE_SYS_SELECT_H',
  compiler.has_header('sys' / 'select.h'))
conf_data.set('PACKETEER_HAVE_SYS_STAT_H',
  compiler.has_header('sys' / 'stat.h'))
conf_data.set('PACKETEER_HAVE_SYS_TIME_H',
  compiler.has_header('sys' / 'time.h'))
conf_data.set('PACKETEER_HAVE_SYS_EPOLL_H',
  compiler.has_header('sys' / 'epoll.h'))
conf_data.set('PACKETEER_HAVE_SYS_UN_H',
  compiler.has_header('sys' / 'un.h'))
conf_data.set('PACKETEER_HAVE_SYS_SOCKET_H',
  compiler.has_header('sys' / 'socket.h'))
conf_data.set('PACKETEER_HAVE_SYS_IOCTL_H',
  compiler.has_header('sys' / 'ioctl.h'))

conf_data.set('PACKETEER_HAVE_ARPA_INET_H',
  compiler.has_header('arpa' / 'inet.h'))

conf_data.set('PACKETEER_HAVE_NET_IF_H',
  compiler.has_header('net' / 'if.h'))

conf_data.set('PACKETEER_HAVE_NETINET_IN_H',
  compiler.has_header('netinet' / 'in.h'))

conf_data.set('PACKETEER_HAVE_WINSOCK2_H',
  compiler.has_header('winsock2.h'))
conf_data.set('PACKETEER_HAVE_WS2TCPIP_H',
  compiler.has_header('ws2tcpip.h'))
have_afunix = compiler.has_header('afunix.h')
conf_data.set('PACKETEER_HAVE_AFUNIX_H', have_afunix)

conf_data.set('PACKETEER_HAVE_LINUX_UN_H',
  compiler.has_header('linux' / 'un.h'))
conf_data.set('PACKETEER_HAVE_LINUX_IF_TUN_H',
  compiler.has_header('linux' / 'if_tun.h'))
conf_data.set('PACKETEER_HAVE_LINUX_SOCKIOS_H',
  compiler.has_header('linux' / 'sockios.h'))


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
conf_data.set('PACKETEER_HAVE_EPOLL_CREATE1', have_epoll_create)
summary('epoll', have_epoll_create, bool_yn: true, section: 'I/O subsystems')


have_select = compiler.compiles('''
#include <sys/select.h>
#include <string.h>

int main(int, char**)
{
  int ret = select(0, NULL, NULL, NULL, NULL);
}
''', name: 'select()')
conf_data.set('PACKETEER_HAVE_SELECT', have_select)
summary('select', have_select, bool_yn: true, section: 'I/O subsystems')


have_pselect = compiler.compiles('''
#include <sys/select.h>
#include <string.h>

int main(int, char**)
{
  int ret = pselect(0, NULL, NULL, NULL, NULL, NULL);
}
''', name: 'pselect()')
conf_data.set('PACKETEER_HAVE_PSELECT', have_pselect)
summary('pselect', have_pselect, bool_yn: true, section: 'I/O subsystems')


have_poll = compiler.compiles('''
#include <poll.h>
#include <string.h>

int main(int, char**)
{
  int foo = poll(NULL, 0, 0);
}
''', name: 'poll()')
conf_data.set('PACKETEER_HAVE_POLL', have_poll)
summary('poll', have_poll, bool_yn: true, section: 'I/O subsystems')


have_ppoll = compiler.compiles('''
#include <poll.h>
#include <string.h>

int main(int, char**)
{
  int foo = ppoll(NULL, 0, 0, NULL);
}
''', name: 'ppoll()')
conf_data.set('PACKETEER_HAVE_PPOLL', have_ppoll)
summary('ppoll', have_ppoll, bool_yn: true, section: 'I/O subsystems')


have_pollrdhup = compiler.compiles('''
#include <poll.h>
#include <string.h>

int main(int, char**)
{
  int foo = POLLRDHUP;
}
''', name: 'pollrdhup()')
conf_data.set('PACKETEER_HAVE_POLLRDHUP', have_pollrdhup)


have_pollhup = compiler.compiles('''
#include <poll.h>
#include <string.h>

int main(int, char**)
{
  int foo = POLLHUP;
}
''', name: 'pollhup()')
conf_data.set('PACKETEER_HAVE_POLLHUP', have_pollhup)


have_kqueue = compiler.compiles('''
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

int main(int, char **)
{
  int foo = kqueue();
}
''', name: 'kqueue()')
conf_data.set('PACKETEER_HAVE_KQUEUE', have_kqueue)
summary('kqueue', have_kqueue, bool_yn: true, section: 'I/O subsystems')


have_iocp = compiler.compiles('''
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <WinDef.h>

int main(int, char **)
{
  HANDLE iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 3);
}
''', name: 'I/O completion ports')
conf_data.set('PACKETEER_HAVE_IOCP', have_iocp)
summary('iocp', have_iocp, bool_yn: true, section: 'I/O subsystems')


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
conf_data.set('PACKETEER_HAVE_MKNOD', have_mknod)


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
conf_data.set('PACKETEER_HAVE_MKFIFO', have_mkfifo)


have_socketpair = compiler.compiles('''
#include <sys/types.h>
#include <sys/socket.h>

int main(int, char **)
{
  int fd[2];
  int e = socketpair(AF_INET, SOCK_STREAM, 0, fd);
}
''', name: 'socketpair()')
conf_data.set('PACKETEER_HAVE_SOCKETPAIR', have_socketpair)



### Set values from options

default_concurrency = get_option('default_concurrency')
summary('Default scheduler concurrency (threads)', default_concurrency, section: 'Build options')
conf_data.set('PACKETEER_DEFAULT_CONCURRENCY', default_concurrency)

cache_line_size = get_option('cache_line_size')
summary('Cache line size (Bytes)', cache_line_size, section: 'Build options')
conf_data.set('PACKETEER_CACHE_LINE_SIZE', cache_line_size)

event_wait_interval_usec = get_option('event_wait_interval_usec')
summary('Event wait interval (usec)', event_wait_interval_usec, section: 'Build options')
conf_data.set('PACKETEER_EVENT_WAIT_INTERVAL_USEC', event_wait_interval_usec)

event_max = get_option('event_max')
summary('Maximum number of events to dequeue at once', event_max, section: 'Build options')
conf_data.set('PACKETEER_EVENT_MAX', event_max)

io_buffer_size = get_option('io_buffer_size')
summary('I/O buffers (Bytes)', io_buffer_size, section: 'Build options')
conf_data.set('PACKETEER_IO_BUFFER_SIZE', io_buffer_size)

io_signature_size = get_option('io_signature_size')
summary('I/O signature size (Bytes)', io_signature_size, section: 'Build options')
conf_data.set('PACKETEER_IO_SIGNATURE_SIZE', io_signature_size)

io_num_overlapped = get_option('io_num_overlapped')
summary('Initial number of OVERLAPPED entries per handle', io_num_overlapped, section: 'Build options')
conf_data.set('PACKETEER_IO_NUM_OVERLAPPED', io_num_overlapped)

io_overlapped_grow_by = get_option('io_overlapped_grow_by')
summary('Grow number of OVERLAPPED entries by', io_overlapped_grow_by, section: 'Build options')
conf_data.set('PACKETEER_IO_OVERLAPPED_GROW_BY', io_overlapped_grow_by)

configure_file(
  input: 'build-config.h.in',
  output: 'build-config.h',
  configuration: conf_data
)

main_build_dir = declare_dependency(
  include_directories: include_directories('.')
)


##############################################################################
# Dependencies

thread = dependency('threads', required: true)

##############################################################################
# Meson Wrap Dependencies
liberate = subproject('liberate')
clipp = subproject('muellan-clipp')


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
)

install_headers(
  'include' / 'packeteer' / 'version.h',
  'include' / 'packeteer' / 'error.h',
  'include' / 'packeteer' / 'scheduler.h',
  'include' / 'packeteer' / 'registry.h',
  'include' / 'packeteer' / 'resolver.h',
  'include' / 'packeteer' / 'connector.h',
  'include' / 'packeteer' / 'handle.h',
  'include' / 'packeteer' / 'visibility.h',

  subdir: 'packeteer',
)

install_headers(
  'include' / 'packeteer' / 'connector' / 'types.h',
  'include' / 'packeteer' / 'connector' / 'interface.h',
  'include' / 'packeteer' / 'connector' / 'peer_address.h',

  subdir: 'packeteer' / 'connector',
)

install_headers(
  'include' / 'packeteer' / 'scheduler' / 'events.h',
  'include' / 'packeteer' / 'scheduler' / 'types.h',
  'include' / 'packeteer' / 'scheduler' / 'callback.h',

  subdir: 'packeteer' / 'scheduler',
)

install_headers(
  'include' / 'packeteer' / 'ext' / 'connector' / 'filedesc.h',
  'include' / 'packeteer' / 'ext' / 'connector' / 'tuntap.h',

  subdir: 'packeteer' / 'ext' / 'connector',
)


libsrc = [
  'lib' / 'version.cpp',
  'lib' / 'error.cpp',
  'lib' / 'packeteer.cpp',
  'lib' / 'registry.cpp',
  'lib' / 'resolver.cpp',
  'lib' / 'scheduler.cpp',
  'lib' / 'connector.cpp',
  'lib' / 'connector' / 'peer_address.cpp',
  'lib' / 'scheduler' / 'worker.cpp',
  'lib' / 'scheduler' / 'scheduler_impl.cpp',
  'lib' / 'scheduler' / 'io_thread.cpp',
  'lib' / 'thread' / 'tasklet.cpp',
]


if have_epoll_create
  libsrc += ['lib' / 'scheduler' / 'io' / 'posix' / 'epoll.cpp']
endif

if have_select
  libsrc += ['lib' / 'scheduler' / 'io' / 'posix' / 'select.cpp']
endif

if have_poll
  libsrc +=  ['lib' / 'scheduler' / 'io' / 'posix' / 'poll.cpp']
endif

if have_kqueue
  libsrc +=  ['lib' / 'scheduler' / 'io' / 'posix' / 'kqueue.cpp']
endif

if have_iocp
  libsrc +=  [
    'lib' / 'scheduler' / 'io' / 'win32' / 'iocp.cpp',
    'lib' / 'scheduler' / 'io' / 'win32' / 'select.cpp',
    'lib' / 'scheduler' / 'io' / 'win32' / 'win32.cpp',
  ]
endif

posixsrc = [
  'lib' / 'connector' / 'posix' / 'fd.cpp',
  'lib' / 'connector' / 'posix' / 'common.cpp',
  'lib' / 'connector' / 'posix' / 'anon.cpp',
  'lib' / 'connector' / 'posix' / 'fifo.cpp',
  'lib' / 'connector' / 'posix' / 'socket.cpp',
  'lib' / 'connector' / 'posix' / 'tcp.cpp',
  'lib' / 'connector' / 'posix' / 'udp.cpp',
  'lib' / 'connector' / 'posix' / 'local.cpp',
]

winsrc = [
  'lib' / 'win32' / 'handle.cpp',
  'lib' / 'connector' / 'win32' / 'pipe_operations.cpp',
  'lib' / 'connector' / 'win32' / 'socketpair.cpp',
  'lib' / 'connector' / 'win32' / 'io_operations.cpp',
  'lib' / 'connector' / 'win32' / 'common.cpp',
  'lib' / 'connector' / 'win32' / 'anon.cpp',
  'lib' / 'connector' / 'win32' / 'pipe.cpp',
  'lib' / 'connector' / 'win32' / 'socket.cpp',
  'lib' / 'connector' / 'win32' / 'tcp.cpp',
  'lib' / 'connector' / 'win32' / 'udp.cpp',
]

if have_afunix
  winsrc += [
    'lib' / 'connector' / 'win32' / 'local.cpp',
  ]
endif


if host_type == 'posix' or host_type == 'android'
  libsrc += posixsrc
elif host_type == 'win32'
  libsrc += winsrc
endif


lib = library('packeteer', libsrc,
    include_directories: [includes, libincludes],
    dependencies: [
      thread,
      liberate.get_variable('liberate_dep'),
    ],
    link_args: link_args,
    cpp_args: cpp_lib_is_building,
    version: LIB_VERSION,
    soversion: ABI_VERSION,
    install: true)

packeteer_dep = declare_dependency(
    include_directories: [includes],
    dependencies: [
      liberate.get_variable('liberate_dep'),
    ],
    link_with: [lib],
    link_args: link_args,
    version: LIB_VERSION,
)


extsrc = []
extincludes = []

if host_type == 'posix' or host_type == 'android'
  extsrc += [
    # Duplicated, because we're re-using it.
    'lib' / 'connector' / 'posix' / 'common.cpp',
    'lib' / 'connector' / 'posix' / 'fd.cpp',
    # Extension functionality
    'ext' / 'connector' / 'posix' / 'filedesc.cpp',
    'ext' / 'connector' / 'posix' / 'tuntap.cpp',
  ]
endif

if extsrc.length() > 0

  lib_ext = library('packeteer_ext', extsrc,
      include_directories: [includes, extincludes],
      dependencies: [
        packeteer_dep,
      ],
      link_args: link_args,
      link_with: [lib],
      cpp_args: cpp_lib_is_building,
      version: LIB_VERSION,
      soversion: ABI_VERSION,
      install: true)

  packeteer_ext_dep = declare_dependency(
      include_directories: [includes],
      dependencies: [
        packeteer_dep,
      ],
      link_with: [lib_ext],
      link_args: link_args,
      version: LIB_VERSION,
  )
endif


##############################################################################
# Linter, etc.
cppcheck = find_program('cppcheck', required: false)

if cppcheck.found()
  run_target('cppcheck', command: [
    cppcheck.path(),
    '--cppcheck-build-dir=@0@/cppcheck'.format(meson.current_build_dir()),
    '--enable=all', '-v',
    '--suppress=missingIncludeSystem',
    '--inline-suppr',
    '-I', '@0@/include'.format(meson.current_source_dir()),
    '-I', '@0@/lib'.format(meson.current_source_dir()),
    '-I', meson.current_build_dir(),
    '--std=c++17',
    cpp_lib_is_building,
    '--quiet',
    '@0@/lib'.format(meson.current_source_dir()),
  ])
endif

oclint = find_program('oclint', required: false)

if oclint.found()
  oclint_config = custom_target('oclint_config',
      output: '.oclint',
      input: '.oclint',
      command: ['cp', '@INPUT@', '@OUTPUT@'],
      install: false,
  )

  run_target('oclint', command: [
      oclint.path(),
      libsrc,
      '-o', '@0@/oclint.log'.format(meson.current_build_dir()),
      '--',
      '-I', '@0@/include'.format(meson.current_source_dir()),
      '-I', '@0@/lib'.format(meson.current_source_dir()),
      '-I', meson.current_build_dir(),
      '--std=@0@'.format(get_option('cpp_std')),
      cpp_args,
      cpp_lib_is_building,
      '-DOCLINT_IS_RUNNING',
    ],
    depends: oclint_config,
  )
endif


##############################################################################
# Subdirectories
subdir('test')
subdir('examples')
subdir('benchmarks')
