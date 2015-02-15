
Sheet
=====

.. class:: Sheet()

   Class :class:`Sheet` represents a single sheet that stores cells in a 2-dimensional grid address space.

.. attribute:: Sheet.name

   A string representing the name of the sheet object.  This is a read-only
   attribute.

.. method:: Sheet.set_numeric_cell(row, column, value)

   Set numeric value to a cell at specified position.

   * row - 0-based vertical offset from the top-most row.
   * column - 0-based horizontal offset from the left-most column.
   * value - numeric value to assign to the cell.

.. method:: Sheet.set_string_cell(row, column, value)

   Set string value to a cell at specified position.

   * row - 0-based vertical offset from the top-most row.
   * column - 0-based horizontal offset from the left-most column.
   * value - string value to assign to the cell.

.. method:: Sheet.set_formula_cell(row, column, value)

   Set formula expression to a cell at specified position.

   * row - 0-based vertical offset from the top-most row.
   * column - 0-based horizontal offset from the left-most column.
   * value - formula expression to assign to the cell.

.. method:: Sheet.get_numeric_value(row, column)

   Get a numeric value representing the content of a cell at specified
   position.  If the cell is of numeric type, its value is returned.  If it's
   a formula cell, the result of the calculated formula result is returned if
   the result is of numeric type.

   * row - 0-based vertical offset from the top-most row.
   * column - 0-based horizontal offset from the left-most column.

.. method:: Sheet.get_string_value(row, column)

   Get a string value representing the content of a cell at specified
   position.  If the cell is of string type, its value is returned as-is.  If
   it's a formula cell, the result of the calculated formula result is
   returned if the result is of string type.

   * row - 0-based vertical offset from the top-most row.
   * column - 0-based horizontal offset from the left-most column.

.. method:: Sheet.get_formula_expression(row, column)

   Given a formula cell at specified position, get the formula expression
   stored in that cell.

   * row - 0-based vertical offset from the top-most row.
   * column - 0-based horizontal offset from the left-most column.

.. method:: Sheet.erase_cell(row, column)

   Erase the cell at specified position.  The slot at the specified position
   becomes empty afterward.

   * row - 0-based vertical offset from the top-most row.
   * column - 0-based horizontal offset from the left-most column.

