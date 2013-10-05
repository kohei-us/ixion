#!/usr/bin/env python

import sys

def main ():
    if len(sys.argv) < 2:
        return

    for filepath in sys.argv[1:]:
        process_file(filepath)

def process_file (filepath):
    file_in = open(filepath, "rb")
    content = file_in.read()
    file_in.close()

    if content[0] != '/' or content[1] != '*':
        return

    # Find the position of "*/".
    n = len(content)
    has_star = False
    for i in xrange(2, n):
        c = content[i]
        if has_star and c == '/':
            # Dump all after this position.
            if i == n-2:
                return

            file_out = open(filepath, "wb")
            file_out.write(content[i+2:])
            file_out.close()
            return

        has_star = c == '*'

if __name__ == '__main__':
    main()

