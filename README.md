# Saphyr Compiler
[![Build Status](https://github.com/jdm64/saphyr/workflows/Github%20CI/badge.svg)](https://github.com/jdm64/saphyr/actions)
[![Bugs](https://sonarcloud.io/api/project_badges/measure?project=jdm64_saphyr&metric=bugs)](https://sonarcloud.io/dashboard?id=jdm64_saphyr)
[![Coverage Status](https://coveralls.io/repos/github/jdm64/saphyr/badge.svg?branch=master)](https://coveralls.io/github/jdm64/saphyr?branch=master)

A C++ Like compiler using LLVM as a backend.

## License ##

Unless otherwise stated the source code is licensed under the GPLv3 -- see LICENSE.

## Build Dependencies ##

* [FlexC++](https://fbb-git.github.io/flexcpp/)
* [BisonC++](https://fbb-git.github.io/bisoncpp/)
* [Boost](http://www.boost.org/)
  * Program_Options
  * Filesystem
  * System
* [LLVM](http://llvm.org/) 8+
* Make
* C++14 compiler (either GCC or Clang)
* Python 3 (for running tests)
* [Saphyr-libs](https://github.com/jdm64/saphyr-libs) (for tests)
* [Icmake](https://fbb-git.github.io/icmake/) (required if building FlexC++/BisonC++ from source)
* [Bobcat](https://fbb-git.github.io/bobcat/) (required if building FlexC++/BisonC++ from source)

### Debian/Ubuntu ###

Debian 10 (Buster) and Ubuntu 16.04 (Xenial) or newer have all the required packages in their repositories.

`sudo apt-get install flexc++ bisonc++ make llvm-dev libboost-program-options-dev libboost-filesystem-dev libboost-system-dev clang python3`

### Gentoo ###

A local portage overlay containing bobcat/flexc++/bisonc++ is provided in `scripts/portage`.
After adding the overlay run `emerge saphyr` to install all required dependencies.

### Arch ###

FlexC++/BisonC++ can be found in the AUR along with their dependencies icmake and libbobcat.
The other dependencies can be installed using:

`pacman -S make llvm boost clang`

### Other Linux ###

Install: `make, clang, llvm, boost and python3` for your Linux distribution. If your distribution doesn't
have `flexc++/bisonc++` then you can use the `jdm64/saphyr` docker image to build the frontend by running:

`sudo make frontend-docker`

NOTE: On Fedora you must disable SELinux (`sudo setenforce 0`) or you will get a permission error.

## Build Instructions ##

Run `make` in the src directory and it will build the compiler binary `saphyr`.
