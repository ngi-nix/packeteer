# Contributing

We love merge requests from everyone. By participating in this project, you
agree to abide by the following documents:

* [Code of Conduct](https://gitlab.com/interpeer/packeteer/-/blob/master/CODE_OF_CONDUCT.md)
* [License](https://gitlab.com/interpeer/packeteer/-/blob/master/LICENSE)
* [Developer Certificate of Origin](https://gitlab.com/interpeer/packeteer/-/blob/master/DCO.txt)
  as applied to the license above.
* Additionally, you assign all copyright to the project maintainer, Jens
  Finkhaeuser. See below.

## Licensing

Let's digest that. On a personal note, I know assigning me copyright sucks.
It's temporary.

I'm starting a public interest company with the mission of driving the
Interpeer project in the next few months. I'll re-assign copyright to that
company. In future, the line above will read that you assign all copyright
to that company.

A public interest company mixes the non-profit structure with a business
structure. That means it *must spend all its earnings on it's mission*,
which is the mission of the Interpeer project. The upshot is that this
means the project can provide donation receipts for its fundraising, but
also issue commercial licenses. And it can do that without asking all
individual contributors.

In the meantime, all of your contributions are *still* free, because the
license file includes the GPL.

For me to get rich off your work would mean to renege on my promise here,
which means you could easily sue me, because right here is a written
record of it all. So let's not do this.

Let the record show that I commit to that course of action.

(FWIW, I expect few contributions until the company is formed; this
 entire arrangement is probably not going to have any impact at all.)

## Fork Me!

Fork, then clone the repo:

    git clone git@gitlab.com:your-username/packeteer.git

## Setup

Install meson, the build system. The best way to do that is to install it
via Python's package manager, also because there are supplemental python
scripts.

* Install [Python](https://www.python.org/). Make sure you install the
  `pip` package manager as well; this is the default. Make sure it's a
  version of Python 3, because 2 is no longer maintained.
  * On Windows, you might want to add both the directory holding the
    the `python.exe` file as well as it's sibling, the `Scripts` directory
    to your `%PATH%`.
  * On \*NIX, it tends to be the case that when `python` is in your path,
    so are the scripts it may install.
* Install [meson](https://mesonbuild.com/):
  ```bash
  $ pip install meson [--user]
  ```
  We recommend the `--user` flag, because globally installing Python
  packages can lead to version conflicts. If you want to be even more
  conservative about this, check out [virtualenv](https://virtualenv.pypa.io/en/stable/).
* You probably also want to install [ninja](https://ninja-build.org/). It's
  meson's default backend. It's also available on most operating systems. On
  Windows, it ships with Visual Studio.
* On Windows, make sure to run `vcvarsall.bat` with the Windows SDK settings
  you want. Meson will find the C++ compiler and ninja from the environment
  variables set by the script.

## Building

* Create a build directory. Meson won't build directly in the source
  directory, and that's usually a good thing. Let's call it `build`.
* Run meson:
  ```bash
  $ cd build
  $ meson /path/to/sources
  ```
* Run ninja:
  ```bash
  $ cd build
  $ ninja
  ```

## Testing

The meson build system produces a `test` task for ninja, which runs all tests
after building them. Unfortunately for us, that suppresses output of the test
program, and expects each test case to be its own executable.

That may make sense for your project. For us, though, we're using
[googletest](https://github.com/google/googletest) as a unit test runner, which
means the test case status gets lost here. Just run the executables manually.
