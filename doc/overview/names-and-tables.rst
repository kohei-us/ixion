.. highlight:: cpp

.. _names-and-tables:

Named expressions and tables
============================

This page covers two features that let formulas refer to things by name.  A
*named expression* is a sub-expression that has a meaning of its own, or
that comes up in so many formulas that it deserves a name, so that
``SUM(MyData)`` can stand in for ``SUM(Sheet1!$A$1:$A$5)``.  A *table*
gives a name to a block of cells and to each of its columns, so that
``SUM(Table1[Amount])`` can stand in for the range of one column.  Both are
stored in the :cpp:class:`~ixion::model_context`, and both take part in
dependency tracking just like plain references do.

To keep the rest of the code focused on names and tables, let's first
define two helper functions.  The first one puts a formula into a cell:

.. literalinclude:: ../../doc_example/names_and_tables.cpp
   :language: C++
   :start-after: //!code-start: set-formula
   :end-before: //!code-end: set-formula

It parses the formula string into tokens, stores them in the cell, and
registers the cell with the dependency tracker so that the model knows which
cells it references.  The second one calculates a set of newly inserted
formula cells:

.. literalinclude:: ../../doc_example/names_and_tables.cpp
   :language: C++
   :start-after: //!code-start: calculate
   :end-before: //!code-end: calculate

It calls :cpp:func:`~ixion::query_and_sort_dirty_cells` to get the formula
cells that need to be recalculated, sorted in dependency order, and hands
that sequence to :cpp:func:`~ixion::calculate_sorted_cells`, which
calculates them on the calling thread.

With those in place, we create a model with a few sheets and a resolver for
the Excel A1 syntax:

.. literalinclude:: ../../doc_example/names_and_tables.cpp
   :language: C++
   :start-after: //!code-start: setup
   :end-before: //!code-end: setup
   :dedent: 4


Named expressions
-----------------

A named expression is a sequence of formula tokens stored under a name.  You
create the tokens the same way you would for a formula cell, with
:cpp:func:`~ixion::parse_formula_string`, and store them with
:cpp:func:`~ixion::model_context::set_named_expression`.  Called without a
sheet index, as here, it stores the name in the global scope, visible from
every sheet.  Once the name exists, any formula can use it in place of the
expression it stands for:

.. literalinclude:: ../../doc_example/names_and_tables.cpp
   :language: C++
   :start-after: //!code-start: global-name
   :end-before: //!code-end: global-name
   :dedent: 4

.. code-block:: text

    Sheet1!C1 = 15

The position passed along with the tokens is the *origin* of the named
expression: the cell the expression string was initially parsed at.  Ixion
keeps it for one purpose only, which is to turn the tokens back into a string
later, the way a spreadsheet application shows the definition of a name in a
dialog.  It plays no part in calculation.
:cpp:func:`~ixion::model_context::set_named_expression` also comes in
overloads that take no origin, which record the first cell of the first sheet
in its place.

The expression doesn't have to be a range.  Any formula fragment will do,
such as ``(1+2)/3`` or ``PI()*2``, and a named expression may refer to other
named expressions in turn.

When a formula cell is calculated, each name in it is looked up and its
expression is evaluated as a unit in place of the name, so ``MyData*2`` means
the whole expression times two.  The lookup happens at calculation time, not
when the formula is parsed, so you can define the name after the formulas that
reference it.  A name that doesn't exist by the time the formula is calculated
gives the ``#NAME?`` error.

.. todo::

   Diagram: The formula ``SUM(MyData)`` as tokens, with the ``MyData`` token
   pointing at the named expression's own tokens stored in the model.

   File: images/names-and-tables-expansion.svg

To get a named expression back, call
:cpp:func:`~ixion::model_context::get_named_expression`.  It returns a
:cpp:struct:`~ixion::named_expression_t`, which holds the tokens and the
origin.  When printing the tokens back as a string, you can use the origin as
the position to print from, which reproduces the expression as it was
originally written especially when you use the A1-style reference syntax.
When using the R1C1-style reference syntax, where relative references are
written as offsets, the origin position mostly makes no difference.  The
following code:

.. literalinclude:: ../../doc_example/names_and_tables.cpp
   :language: C++
   :start-after: //!code-start: print-expression
   :end-before: //!code-end: print-expression
   :dedent: 4

produces this output:

.. code-block:: text

    MyData = $A$1:$A$5


Relative references in a named expression
-----------------------------------------

The origin has no say in how the expression is calculated.  When the tokens
are spliced into a formula, any relative reference in them is taken relative
to the *formula cell*, not to the origin.  This makes it possible to define
names like "the cell to my left":

.. literalinclude:: ../../doc_example/names_and_tables.cpp
   :language: C++
   :start-after: //!code-start: relative-name
   :end-before: //!code-end: relative-name
   :dedent: 4

.. code-block:: text

    Sheet1!C2 = 70

Here ``A1`` was parsed at B1, so the tokens say "one column to the left".
Used from C2, that lands on B2, which holds 7.  The origin only matters when
you turn the tokens back into a string, where it decides that the reference
prints as ``A1`` rather than, say, ``B2``.


Global and sheet-local names
----------------------------

A named expression is either *global*, visible from every sheet, or *local*
to one sheet.  The overloads of
:cpp:func:`~ixion::model_context::set_named_expression` that take a sheet
index store a sheet-local name.  When a formula uses a name, the sheet-local
names of the formula's own sheet are searched first, and the global names
second:

.. literalinclude:: ../../doc_example/names_and_tables.cpp
   :language: C++
   :start-after: //!code-start: sheet-local
   :end-before: //!code-end: sheet-local
   :dedent: 4

.. code-block:: text

    Sheet1!C1 = 15
    Sheet2!C1 = 60

Both formulas read ``SUM(MyData)``, but on ``Sheet2`` the local definition
wins, while ``Sheet1`` falls back to the global one.
:cpp:func:`~ixion::model_context::get_named_expression` follows the same
rule when you pass it a sheet index.

To go through all the names in a scope, use
:cpp:func:`~ixion::model_context::get_named_expressions_iterator`, with no
argument for the global names or with a sheet index for the local ones.
Here is a function that prints every name the iterator visits:

.. literalinclude:: ../../doc_example/names_and_tables.cpp
   :language: C++
   :start-after: //!code-start: print-names
   :end-before: //!code-end: print-names

And here it is applied to both scopes:

.. literalinclude:: ../../doc_example/names_and_tables.cpp
   :language: C++
   :start-after: //!code-start: iterate-names
   :end-before: //!code-end: iterate-names
   :dedent: 4

.. code-block:: text

    global names:
      LeftCell = A1
      MyData = $A$1:$A$5
    names local to Sheet2:
      MyData = $A$1:$A$3

.. note::

    :cpp:func:`~ixion::register_formula_cell` looks through named
    expressions when it records a formula cell's references, so a change to
    a cell that a name refers to marks the dependent formula cells dirty as
    usual.  Redefining the name itself is another matter: the formula cells
    that use it still hold the references recorded from the old definition
    until you unregister and register them again.


Tables
------

A table is a rectangular range of cells with a name and named columns.  Its
range may start with a header row and end with one or more totals rows,
and the rows in between are the data rows.

.. todo::

   Diagram: A 5x2 block labelled ``Table1`` with the header row, three data
   rows and the totals row bracketed, and the two columns labelled ``Item``
   and ``Amount``.

   File: images/names-and-tables-table-anatomy.svg

Let's put such a block of cells on a sheet:

.. literalinclude:: ../../doc_example/names_and_tables.cpp
   :language: C++
   :start-after: //!code-start: table-data
   :end-before: //!code-end: table-data
   :dedent: 4

The cells alone don't make a table.  You describe the table with a
:cpp:struct:`~ixion::table_t` and hand it to
:cpp:func:`~ixion::model_context::set_table`:

.. literalinclude:: ../../doc_example/names_and_tables.cpp
   :language: C++
   :start-after: //!code-start: set-table
   :end-before: //!code-end: set-table
   :dedent: 4

The name must be unique within the model, the range must be valid, and the
column names are listed in column order.  Ixion doesn't check that the
column names match what the header row says; the header row is just cells.


Table references
----------------

With the table in place, formulas can refer to its parts by name.  A table
reference, commonly known as a *structured reference* in spreadsheet
applications, names the table and a column, optionally narrowed down to one
or more areas: ``#Data``, ``#Headers``, ``#Totals`` or ``#All``.  When no area
is given, the reference means the data rows of that column.  A formula in a
cell that is itself inside the table can leave the table name out:

.. literalinclude:: ../../doc_example/names_and_tables.cpp
   :language: C++
   :start-after: //!code-start: table-refs
   :end-before: //!code-end: table-refs
   :dedent: 4

.. code-block:: text

    Inventory!B5 = 43.5
    Inventory!D1 = 43.5
    Inventory!D2 = 2
    Inventory!D3 = 43.5

The forms a table reference can take are:

* ``Table1[Amount]`` - the data rows of one column
* ``Table1[[Item]:[Amount]]`` - the data rows of a span of columns
* ``Table1[#Headers]``, ``Table1[#Totals]``, ``Table1[#All]`` - a whole area
* ``Table1[[#Headers],[Amount]]`` - an area narrowed to one column
* ``Table1[[#Headers],[#Data],[Amount]]`` - several areas combined
* ``[Amount]`` - a column of the table the formula cell is in

Note that currently only the Excel A1 syntax supports table references.

A table reference is resolved to a cell range whenever it is needed, so it
takes part in dependency tracking like any other reference.  You can do the
same resolution yourself with
:cpp:func:`~ixion::model_context::get_table_range`:

.. literalinclude:: ../../doc_example/names_and_tables.cpp
   :language: C++
   :start-after: //!code-start: table-range
   :end-before: //!code-end: table-range
   :dedent: 4

.. code-block:: text

    Table1[Amount] -> (sheet:2; row:1; column:1)-(sheet:2; row:3; column:1)
    Table1[#All]   -> (sheet:2; row:0; column:0)-(sheet:2; row:4; column:1)

Finally, :cpp:func:`~ixion::model_context::get_table` fetches one table by
name, and :cpp:func:`~ixion::model_context::get_tables` lists the tables on
a sheet:

.. literalinclude:: ../../doc_example/names_and_tables.cpp
   :language: C++
   :start-after: //!code-start: list-tables
   :end-before: //!code-end: list-tables
   :dedent: 4

.. code-block:: text

    table on Inventory: Table1

The complete source code of this example is available
`here <https://gitlab.com/ixion/ixion/-/blob/master/doc_example/names_and_tables.cpp>`_.
