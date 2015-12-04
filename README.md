# Saphyr Compiler
[![Build Status](https://travis-ci.org/jdm64/saphyr.svg)](https://travis-ci.org/jdm64/saphyr)
[![Scan Status](https://scan.coverity.com/projects/4591/badge.svg)](https://scan.coverity.com/projects/4591)
[![Coverage Status](https://coveralls.io/repos/jdm64/saphyr/badge.svg?branch=master)](https://coveralls.io/r/jdm64/saphyr?branch=master)

A C-Like compiler using LLVM as a backend.

## License ##

Unless otherwise stated the source code is licensed under the GPLv3 -- see LICENSE.

## Build Dependencies ##

* [FlexC++](https://fbb-git.github.io/flexcpp/)
* [BisonC++](https://fbb-git.github.io/bisoncpp/)
* [Boost](http://www.boost.org/) (for program_options)
* [LLVM](http://llvm.org/) 3.4+
* Make
* C++11 compiler (either GCC or Clang)
* Python 3 (for running tests)

Note: Building FlexC++/BisonC++ from sources requires the [Icmake](https://fbb-git.github.io/icmake/) build
system and the [Bobcat](https://fbb-git.github.io/bobcat/) library which requires a C++14 compiler.

### Debian/Ubuntu ###

Debian 7 (Wheezy) and Ubuntu 14.04 (Trusty) or newer have all the required packages in their repositories.

`sudo apt-get install flexc++ bisonc++ make llvm-dev libboost-program-options-dev clang python3`

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

Run `make` in the root directory and it will build the compiler binary `saphyr`.
