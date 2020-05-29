##############################################################################
# Benchmarks

# Only build if it's the main project
if not meson.is_subproject()

  #---------------------------
  # Detect 3rdparty libraries

  libevent_dep = compiler.find_library('event',
      required: false,
      has_headers: ['event2' / 'event.h']
  )
  summary('libevent', libevent_dep.found(), bool_yn: true, section: 'Competitors')

  libev_dep = compiler.find_library('ev',
      required: false,
      has_headers: ['ev.h']
  )
  summary('libev', libev_dep.found(), bool_yn: true, section: 'Competitors')

  libuv_dep = compiler.find_library('uv',
      required: false,
      has_headers: ['uv.h']
  )
  summary('libuv', libuv_dep.found(), bool_yn: true, section: 'Competitors')

  have_asio = compiler.check_header('asio.hpp',
      required: false)
  summary('Boost asio', have_asio, bool_yn: true, section: 'Competitors')

  #---------------------------
  # Event processing benchmark

  event_src = [
    'event' / 'main.cpp',
    'event' / 'packeteer.cpp',
  ]
  event_deps = [packeteer_dep]

  if libevent_dep.found()
    event_src += [
      'event' / 'libevent.cpp',
    ]
    event_deps += [libevent_dep]
  endif

  # TODO add other third-party

  executable('bench_event', event_src,
      dependencies: [
        event_deps,
        clipp.get_variable('clipp_dep'),
      ]
  )

endif
