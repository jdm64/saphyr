# Saphyr Compiler
[![Build Status](https://github.com/jdm64/saphyr/workflows/Github%20CI/badge.svg)](https://github.com/jdm64/saphyr/actions)
[![Coverage Status](https://coveralls.io/repos/github/jdm64/saphyr/badge.svg?branch=master)](https://coveralls.io/github/jdm64/saphyr?branch=master)
[![Static Analysis Status](https://api.codacy.com/project/badge/Grade/3bca14628f32436dae4827f0df5c7c73)](https://app.codacy.com/gh/jdm64/saphyr/dashboard)
[![Bugs](https://sonarcloud.io/api/project_badges/measure?project=jdm64_saphyr&metric=bugs)](https://sonarcloud.io/dashboard?id=jdm64_saphyr)

A C++ Like compiler using LLVM as a backend.

## License

Unless otherwise stated the source code is licensed under the GPLv3 -- see LICENSE.

## Build Dependencies

* [FlexC++](https://fbb-git.gitlab.io/flexcpp/)
* [BisonC++](https://fbb-git.gitlab.io/bisoncpp/)
* [Boost](http://www.boost.org/)
  * Program_Options
  * Filesystem
  * System
* [LLVM](http://llvm.org/) 8+
* Make
* C++17 compiler (either GCC or Clang)
* Python 3 (for running tests)
* [Saphyr-libs](https://github.com/jdm64/saphyr-libs) (for tests)

### Debian/Ubuntu

Debian 10 (Buster) and Ubuntu 16.04 (Xenial) or newer have all the required packages in their repositories.

`sudo apt-get install flexc++ bisonc++ make llvm-dev libboost-program-options-dev libboost-filesystem-dev libboost-system-dev clang python3`

### Other Linux (FlexC++/BisonC++)

If your Linux distro doesn't have flexc++/bisonc++ then you can download appimages of these programs:

* [flexc++](https://github.com/jdm64/saphyr/releases/download/master/flexc++-latest-x86_64.AppImage)
* [bisonc++](https://github.com/jdm64/saphyr/releases/download/master/bisonc++-latest-x86_64.AppImage)

You can also use the `jdm64/saphyr` docker image to build the frontend by running:

`sudo make frontend-docker`

NOTE: On Fedora you must disable SELinux (`sudo setenforce 0`) or you will get a permission error.

## Build Instructions

Run `make` in the src directory and it will build the compiler binary `saphyr`.
