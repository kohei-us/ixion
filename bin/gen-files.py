#!/usr/bin/env python3
"""Script to generate files from the .in files the same way autoconf does.

It is to be used only when autoconf is not used to do the build.
"""

import argparse
import os.path
import sys


class FileGenerator(object):

    @staticmethod
    def parse_properties(props):
        d = dict()
        for kv in props:
            key, value = kv.split('=')
            d[key] = value
        return d

    def __init__(self, props, outfiles):
        self.__props = FileGenerator.parse_properties(props)
        self.__outfiles = outfiles

    def run(self):
        for outfile in self.__outfiles:
            infile = "{}.in".format(outfile)
            print(infile)
            print(outfile)
            with open(infile, 'r') as f:
                content = f.read()
                self.__write_file(outfile, content)

    def __write_file(self, outfile, src_content):
        with open(outfile, 'w') as f:
            keyword_pos = None
            content_pos = 0
            for i, c in enumerate(src_content):
                if c != '@':
                    continue

                if keyword_pos is None:
                    # new keyword detected.
                    keyword_pos = i
                    # flush content to the destination file.
                    f.write(src_content[content_pos:i])
                else:
                    # keyword span has just ended.
                    key = src_content[keyword_pos+1:i]
                    keyword_pos = None
                    # perform keyword substitution.
                    if key not in self.__props:
                        raise RuntimeError("value for {} not defined!".format(key))
                    f.write(self.__props[key])
                    content_pos = i + 1

            if keyword_pos is not None:
                raise RuntimeError("malformed template file!")

            # flush the rest of the content to the destination file.
            f.write(src_content[content_pos:])


def main():
    parser = argparse.ArgumentParser(description="Generate files from the .in files.")
    parser.add_argument(
        "--properties", type=str, nargs="+",
        required=True,
        help="Comma-separated strings representing multiple key-value pairs.")
    parser.add_argument(
        "--files", type=str, nargs="+",
        help="""Source template files from which to generate final files. Each
        file is expected to end with '.in' but you should not include the '.in'
        suffix in the argument to this script.""")

    args = parser.parse_args()
    generator = FileGenerator(args.properties, args.files)
    generator.run()


if __name__ == "__main__":
    main()
