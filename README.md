# Saphyr Compiler
[![Build Status](https://travis-ci.org/jdm64/saphyr.svg)](https://travis-ci.org/jdm64/saphyr)
[![Scan Status](https://scan.coverity.com/projects/4591/badge.svg)](https://scan.coverity.com/projects/4591)

A C-Like compiler using LLVM as a backend.

## License ##

Unless otherwise stated the source code is licensed under the GPLv3 -- see LICENSE.

## Build Dependencies ##

* [FlexC++](http://flexcpp.sourceforge.net/)
* [BisonC++](http://bisoncpp.sourceforge.net/)
* [LLVM](http://llvm.org/) 3.4+
* C++11 compiler (either GCC or Clang)
* Python 3 (for running tests)

### Debian/Ubuntu ###

`sudo apt-get install flexc++ bisonc++ make llvm-dev clang python3`

## Build Instructions ##

Run `make` in the root directory and it will build the compiler binary `saphyr`.