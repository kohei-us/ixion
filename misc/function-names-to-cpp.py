#!/usr/bin/env python3

import argparse
import enum
import sys


class Mode(enum.Enum):
    HEADER = "header"
    SOURCE = "source"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--mode", "-m", type=Mode, required=True,
        help="Code generation mode. Specify either 'header' or 'source.")
    parser.add_argument(
        "--output", "-o", type=argparse.FileType("w"), help="Output file.")
    parser.add_argument(
        "input_file", metavar="INPUT-FILE", type=str, nargs='*',
        help="Input text file that contains a list of function names one per line.")
    args = parser.parse_args()

    output = args.output if args.output else sys.stdout
    names = set()
    for input_file in args.input_file:
        with open(input_file, 'r') as f:
            names.update([s.strip() for s in f.readlines()])

    names = sorted(names)

    if args.mode == Mode.HEADER:
        output.write("enum class formula_function_t\n")
        output.write("{\n")
        output.write("    func_unknown = 0,\n")
        for name in names:
            name = name.lower()
            output.write(f"    func_{name},\n")
        output.write("};")
    elif args.mode == Mode.SOURCE:
        output.write("// Keys must be sorted.\n")
        output.write("const std::vector<map_type::entry> entries =\n")
        output.write("{\n")
        for name in names:
            name_lower = name.lower()
            output.write(f"    {{ IXION_ASCII(\"{name}\"), formula_function_t::func_{name_lower} }},\n")
        output.write("};\n")
    else:
        print("Unknown mode.", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
