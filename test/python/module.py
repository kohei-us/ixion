#!/usr/bin/env python3

import unittest
import ixion


class ModuleTest(unittest.TestCase):

    def test_column_label(self):
        # Get a single label.
        labels = ixion.column_label(0, 1)
        self.assertEqual(1, len(labels))
        self.assertEqual('A', labels[0])

        # Get multiple labels.
        labels = ixion.column_label(2, 10)
        self.assertEqual(8, len(labels))
        self.assertEqual(labels, ('C','D','E','F','G','H','I','J'))

        # The following start, stop combos should individually raise IndexError.
        tests = (
            (2, 2),
            (2, 0),
            (-1, 10)
        )
        for test in tests:
            with self.assertRaises(IndexError):
                labels = ixion.column_label(test[0], test[1])

        # Keyword arguments should work.
        labels = ixion.column_label(start=2, stop=4)
        self.assertEqual(labels, ('C','D'))



if __name__ == '__main__':
    unittest.main()
