
Sheet
=====

.. class:: Sheet()

   Class :class:`Sheet` represents a single sheet that stores cells in a
   2-dimensional grid address space.  Rows and columns are used to specify a
   position in the grid, and both rows and columns are 0-based, with the
   top-left-most cell having the address of row 0 and column 0.

.. attribute:: Sheet.name

   A string representing the name of the sheet object.  This is a read-only
   attribute.

.. method:: Sheet.set_numeric_cell(row, column, value)

   Set a numeric *value* to a cell at specified *row* and *column* position.

.. method:: Sheet.set_string_cell(row, column, value)

   Set a string *value* to a cell at specified *row* and *column* position.

.. method:: Sheet.set_formula_cell(row, column, value)

   Set a formula expression (*value*) to a cell at specified *row* and *column* position.

.. method:: Sheet.get_numeric_value(row, column)

   Get a numeric value representing the content of a cell at specified *row*
   and *column* position.  If the cell is of numeric type, its value is
   returned.  If it's a formula cell, the result of the calculated formula
   result is returned if the result is of numeric type.

.. method:: Sheet.get_string_value(row, column)

   Get a string value representing the content of a cell at specified *row*
   and *column* position.  If the cell is of string type, its value is
   returned as-is.  If it's a formula cell, the result of the calculated
   formula result is returned if the result is of string type.

.. method:: Sheet.get_formula_expression(row, column)

   Given a formula cell at specified *row* and *column* position, get the
   formula expression stored in that cell.

.. method:: Sheet.erase_cell(row, column)

   Erase the cell at specified *row* and *column* position.  The slot at the
   specified position becomes empty afterward.
