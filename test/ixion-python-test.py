#!/usr/bin/env python

import sys
import os.path
import unittest
import itertools

dirname = os.path.dirname(os.path.abspath(__file__))
sys.path.append(dirname + "/../src/python/.libs")
import ixion

class Test(unittest.TestCase):

    def setUp(self):
        self.doc = ixion.Document()

    def test_append_sheets(self):
        tests = (
            "Normal",      # normal name
            "First Sheet", # white space
            "Laura's",     # single quote
            '"Quoted"'     # double quote
        )

        sheets = []
        for test in tests:
            sh = self.doc.append_sheet(test)
            sheets.append(sh)

        for test, sheet in itertools.izip(tests, sheets):
            self.assertEqual(test, sheet.name)

        try:
            sheets[0].name = "Try to change sheet name"
            self.assertTrue(False, "sheet name attribute should not be writable.")
        except TypeError:
            pass # TypeError is expected when attempting to overwrite sheet name attribute.

    def test_numeric_cell_input(self):
        sh1 = self.doc.append_sheet("Data")

        # Empty cell should yield a value of 0.0.
        check_val = sh1.get_numeric_value(0, 0)
        self.assertEqual(0.0, check_val)

        tests = (
            # row, column, value
            (3, 1, 11.2),
            (4, 1, 12.0),
            (6, 2, -12.0),
            (6, 3, 0.0)
        )

        for test in tests:
            sh1.set_numeric_cell(test[0], test[1], test[2]) # row, column, value
            check_val = sh1.get_numeric_value(column=test[1], row=test[0]) # swap row and column
            self.assertEqual(test[2], check_val)


if __name__ == '__main__':
    unittest.main()
