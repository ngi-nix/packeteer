The code is organized as follows:

- `.cpp` files for a public header is in the same relative path, e.g.
  `include/packeteer/version.h` corresponds to `lib/version.cpp`.
- Purely internal files are sorted into subdirectories:
  - `posix` for files only built on POSIX-ish operating systems. The folder may
    contain subdirectories for grouping files (below).
  - `win32` for files only built on windows derivates. The folder may contain
    subdirectories for grouping files (below).
  - Related files may be grouped into subdirectories, i.e. all I/O subsystem
    implementations exit in the `io` folder.
- With the exception of the `posix` and `win32` folders, the relative path of
  the file corresponds to the namespace within the file:
  - namespace `packeteer` is in e.g. files `include/packeteer/*.h` and
    `lib/*.cpp`.
  - namespace `packeteer::net` is in e.g. files `include/packeteer/net/*.h`
    and `lib/net/*.cpp`
  - The same applies to grouped files. The `io` folder contains files defining
    symbols in the `packeteer::io` namespace.

In order to control visibility:

- With all that said, though, *only* symbols in public headers are exported.
- None of the *internal* grouping namespaces will contain visible symbols,
  making an explicit `detail` namespace unnecessary for those symbols,
  especially in the file path. But if such a namespace is desired within a
  source file, that's of course fine.

