#!/usr/bin/env python

import sys

license_lines = """/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
"""

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
    file_out.write(license_lines)
    file_out.write(content)
    file_out.close()

if __name__ == '__main__':
    main()

