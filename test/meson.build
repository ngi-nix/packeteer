##############################################################################
# Tests

# Tests in a subproject are complicicated. You need all the compile and
# link flags from the enclosing project, etc.
if not meson.is_subproject()
  gtest = subproject('gtest')

  # See https://github.com/google/googletest/issues/813
  test_args = []
  if host_machine.system() == 'cygwin'
    test_args += ['-D_POSIX_C_SOURCE=200809L']
  endif

  # Google test issues this warning; disable it in *test* code only.
  if compiler_id == 'msvc'
    test_args = [
      '/wd4389',
    ]
  endif

  # We're building two tests:
  # - public_tests include *only* public headers
  # - private_tests include private headers

  public_test_src = [
    'public' / 'test_registry.cpp',
    'public' / 'test_resolver.cpp',
    'public' / 'test_error.cpp',
    'public' / 'test_error.cpp',
    'public' / 'test_handle.cpp',
    'public' / 'test_scheduler_callback.cpp',
    'public' / 'test_scheduler.cpp',
    'public' / 'test_connector_peer_address.cpp',
    'public' / 'test_connector.cpp',
    'runner.cpp',
  ]

  private_test_src = [
    'private' / 'test_connector_util.cpp',
    'private' / 'test_scheduler_containers.cpp',
    'private' / 'test_io_thread.cpp',
    'runner.cpp',
  ]

  if host_type == 'win32'
    private_test_src += [
      'private' / 'test_pipe_operations.cpp',
      'private' / 'test_io_operations.cpp',
      'private' / 'test_socketpair.cpp',
    ]
  endif

  ext_test_src = [
    'ext' / 'test_connector_filedesc.cpp',
    'ext' / 'test_connector_tuntap.cpp',
    'runner.cpp',
  ]

  public_tests = executable('public_tests', public_test_src,
      dependencies: [
        main_build_dir, # XXX private headers include the build config
        packeteer_dep,
        gtest.get_variable('gtest_dep'),
        liberate.get_variable('liberate_dep'),
      ],
      cpp_args: test_args,
  )
  test('public_tests', public_tests)

  # Due to symbol visibility, private tests won't link for non-debug builds
  if bt in ['debug', 'debugoptimized']
    private_tests = executable('private_tests', private_test_src,
        include_directories: [libincludes],  # Also private headers
        dependencies: [
          main_build_dir, # XXX private headers include the build config
          packeteer_dep,
          gtest.get_variable('gtest_dep'),
          liberate.get_variable('liberate_dep'),
        ],
        cpp_args: test_args,
    )
    test('private_tests', private_tests)
  endif

  if extsrc.length() > 0
    ext_tests = executable('ext_tests', ext_test_src,
        dependencies: [
          packeteer_ext_dep,
          gtest.get_variable('gtest_dep'),
          liberate.get_variable('liberate_dep'),
        ],
        cpp_args: test_args,
    )
    test('ext_tests', ext_tests)
  endif

endif
