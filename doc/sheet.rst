
Sheet
=====

.. class:: Sheet()

   Class :class:`Sheet` represents a single sheet that stores cells in a 2-dimensional grid address space.

The set_numeric_cell() method
-----------------------------
.. method:: Sheet.set_numeric_cell(row, column, value)

   Set numeric value to a cell at position specified.

   * row - 0-based vertical offset from the top-most row.
   * column - 0-based horizontal offset from the left-most column.
   * value - numeric value to assign to the cell.

The set_string_cell() method
----------------------------
.. method:: Sheet.set_string_cell(row, column, value)

The set_formula_cell() method
-----------------------------
.. method:: Sheet.set_formula_cell(row, column, value)

The get_numeric_value() method
------------------------------
.. method:: Sheet.get_numeric_value(row, column)

The get_string_value() method
-----------------------------
.. method:: Sheet.get_string_value(row, column)

The get_formula_expression() method
-----------------------------------
.. method:: Sheet.get_formula_expression(row, column)

The erase_cell() method
-----------------------
.. method:: Sheet.erase_cell(row, column)
