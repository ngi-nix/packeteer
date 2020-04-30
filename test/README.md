Tests
=====

- `public` - tests of the public API. Works for release builds.
- `private` - tests of internal APIs. Should not work, and therefore isn't
  built for release, because internal APIs don't have visibility in release
  builds.
