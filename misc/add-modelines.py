#!/usr/bin/env python

import sys

top_line = "/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */"
bottom_line = "/* vim:set shiftwidth=4 softtabstop=4 expandtab: */"

def main ():
    if len(sys.argv) < 2:
        return

    for filepath in sys.argv[1:]:
        process_file(filepath)

def process_file (filepath):
    file_in = open(filepath, "rb")
    content = file_in.read()
    file_in.close()

    file_out = open(filepath, "wb")
    file_out.write(top_line)
    file_out.write("\n")
    file_out.write(content)
    file_out.write(bottom_line)
    file_out.write("\n")
    file_out.close()

if __name__ == '__main__':
    main()

