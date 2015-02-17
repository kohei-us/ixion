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

        self.assertEqual(tests, self.doc.get_sheet_names())

        for i, test in enumerate(tests):
            # get sheet by index.
            sh = self.doc.get_sheet(i)
            self.assertEqual(test, sh.name)

        for test in tests:
            # get sheet by name.
            sh = self.doc.get_sheet(test)
            self.assertEqual(test, sh.name)

        try:
            sheets[0].name = "Try to change sheet name"
            self.assertTrue(False, "sheet name attribute should not be writable.")
        except TypeError:
            pass # TypeError is expected when attempting to overwrite sheet name attribute.
        except:
            self.assertTrue(False, "Wrong exception has been raised")

        # Trying to insert a new sheet with an existing name should fail.
        try:
            sh = self.doc.append_sheet(tests[0])
            self.assertTrue(False, "Trying to insert a new sheet with an existing sheet name should fail")
        except ixion.DocumentError:
            # This is expected.
            pass

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

    def test_string_cell_input(self):
        sh1 = self.doc.append_sheet("Data")

        # Empty cell should yield an empty string.
        check_val = sh1.get_string_value(0, 0)
        self.assertEqual("", check_val)

        tests = (
            # row, column, value
            (0, 0, "normal string"),  # normal string
            (1, 0, "A1+B1"),          # string that looks like a formula expression
            (2, 0, "'single quote'"), # single quote
            (3, 0, "80's music"),     # single quote
            (4, 0, '"The" Music in the 80\'s'), # single and double quotes mixed
        )

        for test in tests:
            sh1.set_string_cell(test[0], test[1], test[2]) # row, column, value
            check_val = sh1.get_string_value(column=test[1], row=test[0]) # swap row and column
            self.assertEqual(test[2], check_val)

    def test_formula_cell_input(self):
        sh1 = self.doc.append_sheet("Data")
        sh1.set_formula_cell(0, 0, "12*3")
        try:
            val = sh1.get_numeric_value(0, 0)
            self.assertTrue(False, "TypeError should have been raised")
        except TypeError:
            # TypeError is expected when trying to fetch numeric value from
            # formula cell before it is calculated.
            pass

        self.doc.calculate()
        val = sh1.get_numeric_value(0, 0)
        self.assertEqual(12*3, val)

    def test_formula_cell_recalc(self):
        sh1 = self.doc.append_sheet("Data")
        sh1.set_numeric_cell(0, 0, 1.0)
        sh1.set_numeric_cell(1, 0, 2.0)
        sh1.set_numeric_cell(2, 0, 4.0)
        sh1.set_formula_cell(3, 0, "SUM(A1:A3)")

        # initial calculation
        self.doc.calculate()
        val = sh1.get_numeric_value(3, 0)
        self.assertEqual(7.0, val)

        # recalculation
        sh1.set_numeric_cell(1, 0, 8.0)
        self.doc.calculate()
        val = sh1.get_numeric_value(3, 0)
        self.assertEqual(13.0, val)

        # add another formula cell and recalc.
        sh1.set_formula_cell(0, 1, "A1+15")
        sh1.set_numeric_cell(0, 0, 0.0)
        self.doc.calculate()
        val = sh1.get_numeric_value(0, 1)
        self.assertEqual(15.0, val)
        val = sh1.get_numeric_value(3, 0)
        self.assertEqual(12.0, val)

    def test_formula_cell_recalc2(self):
        sh1 = self.doc.append_sheet("Data")
        sh1.set_numeric_cell(4, 1, 12.0) # B5
        sh1.set_formula_cell(5, 1, "B5*2")
        sh1.set_formula_cell(6, 1, "B6+10")

        self.doc.calculate()
        val = sh1.get_numeric_value(4, 1)
        self.assertEqual(12.0, val)
        val = sh1.get_numeric_value(5, 1)
        self.assertEqual(24.0, val)
        val = sh1.get_numeric_value(6, 1)
        self.assertEqual(34.0, val)

        # Delete B5 and check.
        sh1.erase_cell(4, 1)
        self.doc.calculate()
        val = sh1.get_numeric_value(4, 1)
        self.assertEqual(0.0, val)
        val = sh1.get_numeric_value(5, 1)
        self.assertEqual(0.0, val)
        val = sh1.get_numeric_value(6, 1)
        self.assertEqual(10.0, val)

    def test_formula_cell_string(self):
        sh1 = self.doc.append_sheet("MyData")
        sh1.set_string_cell(1, 1, "My precious string")  # B2
        sh1.set_formula_cell(1, 2, "B2")  # C2
        sh1.set_formula_cell(2, 2, "concatenate(B2, \" is here\")")  # C3
        self.doc.calculate()
        self.assertEqual("My precious string", sh1.get_string_value(1, 1))
        self.assertEqual("My precious string", sh1.get_string_value(1, 2))
        self.assertEqual("My precious string is here", sh1.get_string_value(2, 2))

    def test_detached_sheet(self):
        # You can't set values to a detached sheet that doesn't belong to a
        # Document object.
        sh = ixion.Sheet()
        try:
            sh.set_numeric_cell(1, 1, 12)
            self.assertTrue(False, "failed to raise a SheetError.")
        except ixion.SheetError:
            pass # expected

        try:
            sh.set_string_cell(2, 2, "String")
            self.assertTrue(False, "failed to raise a SheetError.")
        except ixion.SheetError:
            pass # expected

        try:
            sh.set_formula_cell(2, 2, "A1")
            self.assertTrue(False, "failed to raise a SheetError.")
        except ixion.SheetError:
            pass # expected

        try:
            sh.erase_cell(2, 1)
            self.assertTrue(False, "failed to raise a SheetError.")
        except ixion.SheetError:
            pass # expected

        try:
            val = sh.get_numeric_value(2, 1)
            self.assertTrue(False, "failed to raise a SheetError.")
        except ixion.SheetError:
            pass # expected

        try:
            s = sh.get_string_value(2, 1)
            self.assertTrue(False, "failed to raise a SheetError.")
        except ixion.SheetError:
            pass # expected

        try:
            expr = sh.get_formula_expression(2, 1)
            self.assertTrue(False, "failed to raise a SheetError.")
        except ixion.SheetError:
            pass # expected


if __name__ == '__main__':
    unittest.main()
