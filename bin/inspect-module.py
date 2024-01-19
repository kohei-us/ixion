#!/usr/bin/env python3
########################################################################
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
########################################################################

import importlib.machinery
import importlib.util
import argparse
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description="Simple utility to load a custom module from filepath.")
    parser.add_argument("path", type=Path, help="Path to the module to load.")
    args = parser.parse_args()

    name = args.path.stem
    print(f"module path: {args.path.resolve()}")
    print(f"module name: {name}")

    loader = importlib.machinery.ExtensionFileLoader(name, str(args.path))
    print(f"loader: {loader}")

    spec = importlib.util.spec_from_file_location(name, str(args.path), loader=loader)
    print(f"spec: {spec}")

    mod = importlib.util.module_from_spec(spec)
    print(f"module from spec: {mod}")

    print("symbols available from this module:")

    for sym in dir(mod):
        print(f"  - {sym}")


if __name__ == "__main__":
    main()


