The code is organized as follows:

- `.cpp` files for a public header is in the same relative path, e.g.
  `include/packeteer/version.h` corresponds to `lib/version.cpp`.
- Purely internal files are sorted into subdirectories:
  - `posix` for files only built on POSIX-ish operating systems. The folder may
    contain subdirectories for grouping files (below).
  - `win32` for files only built on windows derivates. The folder may contain
    subdirectories for grouping files (below).
  - Related files may be grouped into subdirectories, i.e. all I/O subsystem
    implementations exist in the `scheduler/io` folder.
- Include guards mirror the file name (for headers).
- Namespaces are a little more flexible than that:
  - Public headers within subdirectories (e.g. `net`, `util`) tend to have
    namespaces corresponding to the relative path.
  - This does not apply to scheduler and connector related types.
  - A `detail` namespace is optional. A lot of implementation details can be
    hidden there.

In order to control visibility:

- With all that said, though, *only* symbols in public headers are exported.
- None of the *internal* grouping namespaces will contain visible symbols,
  making an explicit `detail` namespace unnecessary for those symbols,
  especially in the file path. But if such a namespace is desired within a
  source file, that's of course fine.

