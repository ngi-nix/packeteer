##############################################################################
# Examples

# Only build if it's the main project
if not meson.is_subproject()

  executable('echoserver', ['echoserver' / 'main.cpp'],
    dependencies: [packeteer_dep]
  )

  executable('echoclient', ['echoclient' / 'main.cpp'],
    dependencies: [packeteer_dep]
  )

endif
