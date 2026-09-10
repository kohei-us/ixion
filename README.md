# Ixion

Ixion is a general purpose formula parser, interpreter, formula cell dependency
tracker and spreadsheet document model backend all in one package.

[![License: MPL 2.0](https://img.shields.io/badge/License-MPL_2.0-brightgreen.svg)](https://opensource.org/licenses/MPL-2.0)
[![Documentation](https://readthedocs.org/projects/ixion/badge/?version=latest)](https://ixion.readthedocs.io/en/latest/)
[![pipeline status](https://gitlab.com/ixion/ixion/badges/master/pipeline.svg)](https://gitlab.com/ixion/ixion/-/commits/master)
![C++](https://img.shields.io/badge/C%2B%2B-20-blue?logo=c%2B%2B)

## Overview

Ixion calculates the results of formula expressions stored in the cells of a
multi-sheet spreadsheet document.  Formula cells can reference each other, and
Ixion tracks their dependencies and calculates them in the right order.  After
the initial calculation, it re-calculates only the cells affected by later
modifications.  Calculation can run single-threaded or across an arbitrary
number of threads.

You can use Ixion as a complete formula engine backend with its own cell
storage, or use only its parser to tokenize formula expressions.

## Features

* Multi-sheet document model storing numeric, string, boolean and formula
  cells.
* Formula parsing and printing with Excel A1, Excel R1C1, LibreOffice Calc A1,
  ODFF and ODF cell-range-address style name resolvers.
* Cell, range and 3D references, named expressions and table references.
* Inline strings and inline arrays.
* Dependency tracking for both full calculation and partial re-calculation.
* Threaded calculation with an arbitrary number of threads.
* Volatile functions.
* Formula groups that share one set of formula tokens across cells.
* Sheet copy with copy-on-write cell storage.
* Sheet views that can be sorted independently of their base sheets.
* C++ and Python APIs.

## Features known to be missing

* More built-in functions.
* Custom functions defined in the caller program.
* External references.
* Implicit intersection.

## Requirements

Ixion is written in C++20 and depends on the [boost](https://boost.org) and
[mdds](https://gitlab.com/mdds/mdds) libraries.

## Documentation

* [Official documentation](https://ixion.readthedocs.io/en/latest/), which
  includes overview pages and the C++ and Python API references.

## Installation

Please refer to the [CONTRIBUTING.md](CONTRIBUTING.md) file for build and
installation instructions.

## Download source packages

Please refer to the [Releases](https://gitlab.com/ixion/ixion/-/releases) page.

## License

Ixion is licensed under the [Mozilla Public License 2.0](LICENSE).
