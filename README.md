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
Please refer to the [[Download]] page for source packages.

## Python binding
Starting with version 0.9.0, Ixion provides Python binding for those who wish
to use Ixion from Python scripts.  Refer to this
[documentation](http://kohei.us/files/ixion/pydoc/current) for more details on
how to use Ixion's Python API.
