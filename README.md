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
are avoided as much as possible.  It makes extensive use of the [boost
library](http://boost.org) to achieve portability where possible.

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
* Name resolution using A1-style references.
* Support 2D cell references and named expressions.
* Support range references.
* Dependency tracking during both full calculation and partial re-calculation.
* Inline strings.
* Volatile functions. The framework for volatile functions is implemented. We
  just need to implement more functions.

## Features in progress
* 3D cell and range references - Initial support is in place.  Needs more
  testing.
* Standard C/C++ interface for external applications - It's there, but needs
  to be stabilized.

## Planned features
* Matrix support.
  * Inline matrix.
  * Matrix as formula out from a single function.
  * Jump matrix - range input to function that expects a scalar input, and
    generate matrix output.
* Support for R1C1 style references.
* More built-in functions.
* Support for custom functions defined in the caller program.
* Support for external references.

## Download

| Version | Release Date | Download | Check Sum |
|---------|--------------|----------|-----------|
| 0.9.1 | 2015-04-05 | [libixion-0.9.1.tar.xz](http://kohei.us/files/ixion/src/libixion-0.9.1.tar.xz) | md5sum: d292f6d62847f2305178459390842eac<br/>sha1sum: 8ae071e7e89784933caadeffc16ed7b0764350a9 |
| 0.9.0 | 2015-02-16 | [libixion-0.9.0.tar.xz](http://kohei.us/files/ixion/src/libixion-0.9.0.tar.xz) | md5sum: 26f293e708513dea5e6e25e9232a7400<br/>sha1sum: 4f97682546236aee686e86293f9890d79f25cf23 |
| 0.7.0 | 2013-12-13 | [libixion-0.7.0.tar.bz2](http://kohei.us/files/ixion/src/libixion-0.7.0.tar.bz2) | md5sum: 000157117801f9507f34b26ba998c4d1<br/>sha1sum: 99b8f9f49078ef7e15280f5c73dff639a6e9472c |
| 0.5.0 | 2013-03-27 | [libixion-0.5.0.tar.bz2](http://kohei.us/files/ixion/src/libixion-0.5.0.tar.bz2) | md5sum: ebaeab9ffe1e6bd68b2a20bfa430b3af<br/>sha1sum: 99290ed5aa2ab2338ba04737210256c48885107c |
| 0.3.0 | 2011-11-01 | [libixion_0.3.0.tar.bz2](http://kohei.us/files/ixion/src/libixion_0.3.0.tar.bz2) | md5sum: 96a36a0016f968a5a7c4b167eeb1643b<br/>sha1sum: ac1fa915303ed8492ac50d6f0aa4d974e8405954 |

## Python binding
Starting with version 0.9.0, Ixion provides Python binding for those who wish
to use Ixion from Python scripts.  Refer to this
[documentation](http://kohei.us/files/ixion/pydoc/current) for more details on
how to use Ixion's Python API.