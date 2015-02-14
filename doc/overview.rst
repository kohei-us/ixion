
Overview
========

Let's go over very quickly how to create a document and populate some cells inside spreadsheet.

First, you need to import ixion module and create a new document.

::

    >>> import ixion
    >>> doc = ixion.Document()

Since your newly-created document has no sheet at all, you need to insert one.

::

    >>> sheet1 = doc.append_sheet("MySheet1")

The :meth:`append_sheet` method takes a sheet name string as an argument (which in
this case is "MySheet1") and returns an object representing the sheet that has
just been inserted.  This sheet object allows access to the sheet
name via its ``name`` attribute.

::

    >>> print(sheet1.name)
    'MySheet1'

.. warning:: This attribute is read-only; you'll get a :exc:`TypeError` if you
             attempt to assign a new value to it.

Now that you have a sheet object, let's go over how to put new cell values into
the sheet.  The sheet object provides several methods to set new cell values
and also to retrieve them afterward.

::

    >>> sheet1.set_numeric_cell(0, 0, 12.3)  # Set 12.3 to cell A1.
    >>> sheet1.get_numeric_value(0, 0)
    12.3
    >>> sheet1.set_string_cell(1, 0, "My string")  # Set "My string" to cell A2.
    >>> sheet1.get_string_value(1, 0)
    'My string'

The setters take 3 arguments: the first one is a 0-based row index, the second
one is a column index (also 0-based), and the third one is the new cell value.
You can also pass these arguments by name as follows:

::

    >>> sheet1.set_string_cell(row=1, column=0, value="My string")

Let's insert a formula expression next.

::

    >>> sheet1.set_formula_cell(0, 1, "A1*100")  # B1
    >>> sheet1.set_formula_cell(1, 1, "A2")      # B2

.. note:: When setting a formula expression to a cell, you don't need to start
          your formula expression with a '=' like you would when you are
          entering a formula in a spreadsheet application.

Now, the formula cells don't get calculated automatically as you enter them;
you need to explicitly tell the document to calculate the formula cells via
its :meth:`calculate` method.

::

    >>> doc.calculate()

Now all the formula cells in this document have been calculated.














