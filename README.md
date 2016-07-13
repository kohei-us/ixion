Ixion is a general purpose formula parser & interpreter that can calculate
multiple named targets, or "cells".

## Overview
The goal of this project is to create a library for calculating the
results of formula expressions stored in multiple named targets, or
"cells".  The cells can be referenced from each other, and the library
takes care of resolving their dependencies automatically upon calculation.
The caller can run the calculation routine either in a single-threaded
mode, or a multi-threaded mode.  The library also supports re-calculations
where the contents of one or more cells have been modified since the last
calculation, and a partial calculation of only the affected cells need to
be calculated.

## Portability
This library is written with portability in mind; platform specific calls
are avoided as much as possible.  It makes use of the [boost library](http://boost.org)
to achieve portability in some places.

## Performance
Achieving good performance is one of the goals of this project.  As much
care is taken as humanly possible, to attain reasonable performance.

## Threaded calculation
Ixion can perform threaded calculation using arbitrary number of threads,
for both full and partial calculation modes.

## Supported features
* Each calculation session is defined in a plain text file, which is parsed
  and interpreted by the Ixion parser.
* Fully threaded calculation.
* Name resolution using A1- and R1C1-style references.
* Support 2D cell references and named expressions.
* Support range references.
* Support table references.
* 3D cell and range references.
* Dependency tracking during both full calculation and partial re-calculation.
* Inline strings.
* Volatile functions. The framework for volatile functions is implemented. We
  just need to implement more functions.
* C++ API.
* Python API.

## Planned features
* Matrix support.
  * Inline matrix.
  * Matrix as formula out from a single function.
  * Jump matrix - range input to function that expects a scalar input, and
    generate matrix output.
* More built-in functions.
* Support for custom functions defined in the caller program.
* Support for external references.

## Documentation

* [Official API documentation](http://kohei.us/files/ixion/doc/)

## Download source packages

| Version | API Version | Release Date | Download | Check Sum | File Size (bytes) |
|---------|-------------|--------------|----------|-----------|-------------------|
| 0.11.1 | 0.11   | 2016-05-11 | [libixion-0.11.1.tar.xz](http://kohei.us/files/ixion/src/libixion-0.11.1.tar.xz) | sha256sum: c9e9f52580d618fa969fc0293f55af21a9c74bfb802e655c6bf239202f95bede | 366660 |
|        |        |            | [libixion-0.11.1.tar.gz](http://kohei.us/files/ixion/src/libixion-0.11.1.tar.gz) | sha256sum: 9ff20f6370e6f1e86a67d4159123195a875b11f7332b0cf9373ba98c57953916 | 568876 |
| 0.11.0 | 0.11   | 2016-02-13 | [libixion-0.11.0.tar.xz](http://kohei.us/files/ixion/src/libixion-0.11.0.tar.xz) | sha256sum: 97a6e7f2b1fcbff69e76fe4e1df62f1cfcc353820472991e37de00aacb024293 | 365652 |
|        |        |            | [libixion-0.11.0.tar.gz](http://kohei.us/files/ixion/src/libixion-0.11.0.tar.gz) | sha256sum: fd697e9ab334bb1cf0161fab25f46bbbcf517248de9bbc1f684d9854b9b287c0 | 567644 |
| 0.9.1  |        | 2015-04-05 | [libixion-0.9.1.tar.xz](http://kohei.us/files/ixion/src/libixion-0.9.1.tar.xz) | md5sum: d292f6d62847f2305178459390842eac<br/>sha1sum: 8ae071e7e89784933caadeffc16ed7b0764350a9 | - |
| 0.9.0  |        | 2015-02-16 | [libixion-0.9.0.tar.xz](http://kohei.us/files/ixion/src/libixion-0.9.0.tar.xz) | md5sum: 26f293e708513dea5e6e25e9232a7400<br/>sha1sum: 4f97682546236aee686e86293f9890d79f25cf23 | - |
| 0.7.0  |        | 2013-12-13 | [libixion-0.7.0.tar.bz2](http://kohei.us/files/ixion/src/libixion-0.7.0.tar.bz2) | md5sum: 000157117801f9507f34b26ba998c4d1<br/>sha1sum: 99b8f9f49078ef7e15280f5c73dff639a6e9472c | - |
| 0.5.0  |        | 2013-03-27 | [libixion-0.5.0.tar.bz2](http://kohei.us/files/ixion/src/libixion-0.5.0.tar.bz2) | md5sum: ebaeab9ffe1e6bd68b2a20bfa430b3af<br/>sha1sum: 99290ed5aa2ab2338ba04737210256c48885107c | - |
| 0.3.0  |        | 2011-11-01 | [libixion_0.3.0.tar.bz2](http://kohei.us/files/ixion/src/libixion_0.3.0.tar.bz2) | md5sum: 96a36a0016f968a5a7c4b167eeb1643b<br/>sha1sum: ac1fa915303ed8492ac50d6f0aa4d974e8405954 | - |

## Building from source code

Ixion uses autoconf and automake as its build system.  As such, building it
from sources should be familiar to those who are used to these tools.

In short, run the following command:

```bash
./autogen.sh
make
make install
```

at the root directory after either cloning from the repository or unpacking
the source package.

### Build dependencies

Ixion has build-time dependency on the following libraries:

* [boost](http://boost.org)
* [mdds](http://gitlab.com/mdds/mdds)

Note that when you are building from the master branch of the git repository,
it's recommended that you also use the latest mdds source code from its
repository for the build, else you may encounter build issues or test failures.

