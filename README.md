Ixion is a general purpose formula parser, interpreter, formula cell dependency
tracker and spreadsheet document model backend all in one package.

## Overview
The goal of this project is to create a library for calculating the
results of formula expressions stored in multiple formula cells.  The cells
can be referenced by each other, and Ixion resolves their dependencies
automatically upon calculation.  The caller can run the calculation routine
either in a single-threaded mode, or a multi-threaded mode. Ixion also supports
re-calculations where the contents of one or more cells have been modified
since the last calculation, and a partial calculation of only the affected
cells need to be calculated.

## Portability
This library is written with portability in mind; platform specific calls
are avoided as much as possible.  It makes use of modern C++ features and the
[boost library](http://boost.org) to achieve portability.

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
* Matrix support via grouped formulas.

## Features known to be missing
* More built-in functions.
* Custom functions defined in the caller program.
* External references.
* Implicit intersection.

## Documentation

* [Official API documentation](https://ixion.readthedocs.io/en/latest/) for general users of the library.

## Installation

Please refer to the [CONTRIBUTING.md](CONTRIBUTING.md) file for build and
installation instructions.


## Download source packages

Please refer to the [Releases](https://gitlab.com/ixion/ixion/-/releases) page.
The source packages for the older versions are found [here](OLD-DOWNLOADS.md).


