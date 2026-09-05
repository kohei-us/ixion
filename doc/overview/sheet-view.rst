.. highlight:: cpp

Sorting a sheet through a sheet view
====================================

A :cpp:class:`~ixion::sheet_view` is a named view of a sheet whose rows can
get sorted without touching the sheet itself.  Think of the sheet views
feature of a spreadsheet application, where each user sorts the rows of a
shared sheet the way they like while everyone keeps working on the same
underlying data.

A view takes a snapshot of the content of its base sheet when created, and
reads its cell values from that snapshot.  Views never calculate;
formula cells in a view carry the cached results they had on the base sheet
when the snapshot was taken.


Populate the base sheet
-----------------------

Let's start with a small sheet holding a header row, three data rows and a
totals row, and dump it to see what we have:

.. literalinclude:: ../../doc_example/sheet_view.cpp
   :language: C++
   :start-after: //!code-start: populate
   :end-before: //!code-end: populate
   :dedent: 4

The :cpp:func:`~ixion::model_context::dump_sheet` method prints the content
of a sheet as a text grid:

.. code-block:: text

    +---+-------+-------+
    |   | A     | B     |
    +---+-------+-------+
    | 1 | Name  | Score |
    | 2 | bob   | 20    |
    | 3 | amy   | 30    |
    | 4 | cid   | 10    |
    | 5 | Total | 60    |
    +---+-------+-------+


Create a view
-------------

You create a view via :cpp:func:`~ixion::model_context::create_sheet_view`,
which takes the index of the sheet to view and a name for the view.  The
name must be unique among the views of the same sheet.  The model context
owns the view, and the returned reference stays valid until the view gets
removed or the model context gets destroyed:

.. literalinclude:: ../../doc_example/sheet_view.cpp
   :language: C++
   :start-after: //!code-start: create-view
   :end-before: //!code-end: create-view
   :dedent: 4

The view offers the same cell value getters as :cpp:class:`~ixion::model_context`,
except that they take an :cpp:struct:`~ixion::abs_rc_address_t`, a cell
position without a sheet index, since a view is always of one sheet.  Right
after creation the view shows exactly what the base sheet shows:

.. code-block:: text

    A2 in the view: bob


Sort a range
------------

To sort, call :cpp:func:`~ixion::sheet_view::sort` with the range to sort
and one or more sort keys.  Each :cpp:struct:`~ixion::sort_key_t` names a
column within the range and the sort direction as a
:cpp:enum:`~ixion::sort_order_t` value; the direction must always be given.
Here we sort the three data rows by the score column in descending order:

.. literalinclude:: ../../doc_example/sheet_view.cpp
   :language: C++
   :start-after: //!code-start: sort
   :end-before: //!code-end: sort
   :dedent: 4

The rows of the range move as units across all of its columns, while the
header and totals rows outside the range stay put.  The
:cpp:func:`~ixion::sheet_view::dump` method prints the view the same way
:cpp:func:`~ixion::model_context::dump_sheet` prints a sheet, with an extra
``base`` column showing the base sheet row each row of the view comes from:

.. code-block:: text

    +---+------+-------+-------+
    |   | base | A     | B     |
    +---+------+-------+-------+
    | 1 |    1 | Name  | Score |
    | 2 |    3 | amy   | 30    |
    | 3 |    2 | bob   | 20    |
    | 4 |    4 | cid   | 10    |
    | 5 |    5 | Total | 60    |
    +---+------+-------+-------+

The base sheet stays untouched:

.. literalinclude:: ../../doc_example/sheet_view.cpp
   :language: C++
   :start-after: //!code-start: base-untouched
   :end-before: //!code-end: base-untouched
   :dedent: 4

.. code-block:: text

    A2 in the base sheet: bob

The sort is stable.  In ascending order, cells of different types come in
the following order:

1. numeric values
2. strings
3. boolean values, false before true
4. error values
5. empty cells

A descending sort reverses the order of the first four types, while empty
cells always come last regardless of the sort direction.

.. note::

    Empty cells always come last regardless of the sort direction.

Formula cells sort by their cached results.  See
:cpp:func:`~ixion::sheet_view::sort` for the details, including how formula
groups behave when a sort moves their cells.


Map rows between the view and the base sheet
--------------------------------------------

The ``base`` column of the dump comes from the row mapping the view keeps.
You can query it in both directions via
:cpp:func:`~ixion::sheet_view::to_base_row` and
:cpp:func:`~ixion::sheet_view::to_view_row`:

.. literalinclude:: ../../doc_example/sheet_view.cpp
   :language: C++
   :start-after: //!code-start: row-mapping
   :end-before: //!code-end: row-mapping
   :dedent: 4

.. code-block:: text

    view row 1 shows base row 2
    base row 1 is shown at view row 2

Note that row positions are 0-based here, whereas the dump labels rows
from 1.  The mapping reflects the combined effect of all the sorts
applied to the view so far in case multiple sorts are applied.

The view-to-base mapping comes in handy when, in a hypothetical UI, a user
edits a cell in a sheet view and the new value needs to reach the base
sheet.  You would query the position of the corresponding cell in the base
sheet, and apply the same edit there.

Likewise, the base-to-view mapping comes in handy when a user edits a cell
in the base sheet and the change needs to reach every view of that sheet.
You would query the position of the corresponding cell in each view, and
update what the view displays there.

.. note::

   These scenarios are hypothetical.  Ixion itself does not currently
   propagate edits between a sheet and its views; the mapping is there in
   case an application using Ixion needs to support such actions.


Refresh the view
----------------

Since a view is a snapshot, later edits to the base sheet do not
automatically show up in it.  Call
:cpp:func:`~ixion::sheet_view::refresh` to take a fresh snapshot.  When
the view is refreshed, the view takes the current cell values from the
base sheet while preserving the row order even when the new values no
longer match the sort order.

For example, we raise cid's score on the base sheet from 10 to 40 and
refresh:

.. literalinclude:: ../../doc_example/sheet_view.cpp
   :language: C++
   :start-after: //!code-start: refresh
   :end-before: //!code-end: refresh
   :dedent: 4

.. code-block:: text

    +---+------+-------+-------+
    |   | base | A     | B     |
    +---+------+-------+-------+
    | 1 |    1 | Name  | Score |
    | 2 |    3 | amy   | 30    |
    | 3 |    2 | bob   | 20    |
    | 4 |    4 | cid   | 40    |
    | 5 |    5 | Total | 60    |
    +---+------+-------+-------+

The view now shows the new score, but cid stays in the last data row.
Re-sorting happens only when you ask for it.  Sorting the same range again
re-orders the rows as the view currently shows them:

.. literalinclude:: ../../doc_example/sheet_view.cpp
   :language: C++
   :start-after: //!code-start: re-sort
   :end-before: //!code-end: re-sort
   :dedent: 4

.. code-block:: text

    +---+------+-------+-------+
    |   | base | A     | B     |
    +---+------+-------+-------+
    | 1 |    1 | Name  | Score |
    | 2 |    4 | cid   | 40    |
    | 3 |    3 | amy   | 30    |
    | 4 |    2 | bob   | 20    |
    | 5 |    5 | Total | 60    |
    +---+------+-------+-------+


Sort a table
------------

When the sorted rows belong to a table, you can sort by a table column name
instead of a range and a column position.  First, define the table via
:cpp:func:`~ixion::model_context::set_table`.  A :cpp:struct:`~ixion::table_t`
stores the name of the table, the sheet it is on, its entire range including
the header row and the totals rows, its column names, and the number of
totals rows:

.. literalinclude:: ../../doc_example/sheet_view.cpp
   :language: C++
   :start-after: //!code-start: set-table
   :end-before: //!code-end: set-table
   :dedent: 4

Then call :cpp:func:`~ixion::sheet_view::sort_table` with the table name, the
column name to sort by, and the direction.  Only the data rows of the table
move; its header row and totals rows stay in place:

.. literalinclude:: ../../doc_example/sheet_view.cpp
   :language: C++
   :start-after: //!code-start: sort-table
   :end-before: //!code-end: sort-table
   :dedent: 4

.. code-block:: text

    +---+------+-------+-------+
    |   | base | A     | B     |
    +---+------+-------+-------+
    | 1 |    1 | Name  | Score |
    | 2 |    3 | amy   | 30    |
    | 3 |    2 | bob   | 20    |
    | 4 |    4 | cid   | 40    |
    | 5 |    5 | Total | 60    |
    +---+------+-------+-------+


Remove the view
---------------

A view stays alive until the model context that owns it gets destroyed,
unless you remove it earlier via
:cpp:func:`~ixion::model_context::remove_sheet_view`.  Afterwards
:cpp:func:`~ixion::model_context::get_sheet_view` no longer finds it:

.. literalinclude:: ../../doc_example/sheet_view.cpp
   :language: C++
   :start-after: //!code-start: remove-view
   :end-before: //!code-end: remove-view
   :dedent: 4

.. code-block:: text

    view found: false

The complete source code of this example is available
`here <https://gitlab.com/ixion/ixion/-/blob/master/doc_example/sheet_view.cpp>`_.
