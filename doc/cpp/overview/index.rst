
.. highlight:: cpp

Overview
========

Create a model context instance
-------------------------------

When using ixion, the very first step is to create a :cpp:class:`~ixion::model_context`
instance::

    ixion::model_context cxt;

The :cpp:class:`~ixion::model_context` class represents a document model data
store that stores cell values spreading over one or more sheets.  At the time of construction,
the model contains no sheets. So the obvious next step is to insert a sheet::

    // First and foremost, insert a sheet.
    cxt.append_sheet("MySheet");

The :cpp:func:`~ixion::model_context::append_sheet` method will append a new sheet to
the model.  You need to give a name when appending a sheet, and the name must be unique
for each sheet.

.. note::

    Each sheet has a fixed size which cannot be changed once the :cpp:class:`~ixion::model_context`
    object is instantiated.  The default sheet size is 1048576 rows by 16384 columns.  You can
    specify a custom sheet size by passing a desired sheet size value to the
    :cpp:class:`~ixion::model_context` constructor at the time of instantiation.


Populate model context with values
----------------------------------

Now that you have your first sheet inserted, let's put in some numeric values.  In this example,
we'll insert into A1:A10 their respective row positions.  To insert a numeric value, you use
:cpp:func:`~ixion::model_context::set_numeric_cell` which takes the position of the cell as its
first argument and the value to set as its second argument.  You need to use :cpp:class:`~ixion::abs_address_t`
to specify a cell position.

::

    // Now, populate it with some numeric values in A1:A10.
    for (ixion::abs_address_t pos(0, 0, 0); pos.row <= 9; ++pos.row)
    {
        double value = pos.row + 1.0; // Set the row position + 1 as the cell value.
        cxt.set_numeric_cell(pos, value);
    }

Note that, since row and column positions are internally 0-based, we add one to emulate how the row
positions are presented in typical spreadsheet program.

Inserting a string value can be done via :cpp:func:`~ixion::model_context::set_string_cell`.  You can choose
one of three ways.

The first way is to store the string value to an outside buffer (like std::string), and pass a pointer to
that buffer and the size of the string value as in the following::

    // Insert a string value into B2.
    ixion::abs_address_t B2(0, 1, 1);
    std::string s = "This cell contains a string value.";
    cxt.set_string_cell(B2, s.data(), s.size());

Alternatively, you can use this convenience macro :c:macro:`IXION_ASCII` which expands to a char pointer and
its length if you are passing a string literal::

    // Insert a literal string value into B3.
    ixion::abs_address_t B3(0, 2, 1);
    cxt.set_string_cell(B3, IXION_ASCII("This too contains a string value."));

But when you do, please be aware that you can only use this macro in conjunction with string literal only.

The third way is to add your string to the model_context's internal string pool first which will return its
string ID, then store that ID to the model::

    // Insert a string value into B4 via string identifier.
    s = "Yet another string value.";
    ixion::string_id_t sid = cxt.add_string(s.data(), s.size());
    ixion::abs_address_t B4(0, 3, 1);
    cxt.set_string_cell(B4, sid);

The model_context class has two methods for inserting a string to the string pool:
:cpp:func:`~ixion::model_context::add_string` and :cpp:func:`~ixion::model_context::append_string`.  The
:cpp:func:`~ixion::model_context::add_string` method checks for an existing entry with the same string value
upon each insertion attempt, and it will not insert the new value if the value already exists in the pool.
The :cpp:func:`~ixion::model_context::append_string` method, on the other hand, does not check the pool for
an existing value and always inserts the value.  The :cpp:func:`~ixion::model_context::append_string` method
is appropriate if you know all your string entries ahead of time and wish to bulk-insert them.  Otherwise the
:cpp:func:`~ixion::model_context::add_string` method is the right one to use.


Insert a formula cell into model context
----------------------------------------

TBD

::

    // Tokenize formula string first.
    std::unique_ptr<ixion::formula_name_resolver> resolver =
        ixion::formula_name_resolver::get(ixion::formula_name_resolver_t::excel_a1, &cxt);
    s = "SUM(A1:A10)";

TBD

::

    ixion::abs_address_t A11(0, 10, 0);
    ixion::formula_tokens_t tokens = ixion::parse_formula_string(cxt, A11, *resolver, s.data(), s.size());

TBD

::

    // Set the tokens into the model.
    const ixion::formula_cell* cell = cxt.set_formula_cell(A11, std::move(tokens));

TBD

::

    // Register this formula cell for automatic dependency tracking.
    ixion::register_formula_cell(cxt, A11, cell);

TBD

::

    ixion::rc_size_t sheet_size = cxt.get_sheet_size();
    ixion::abs_range_t entire_sheet(0, 0, 0, sheet_size.row, sheet_size.column); // sheet, row, column, row span, column span
    ixion::abs_range_set_t modified_cells{entire_sheet};

TBD

::

    // Determine formula cells that need re-calculation given the modified cells.
    // There should be only one formula cell in this example.
    std::vector<ixion::abs_range_t> dirty_cells = ixion::query_and_sort_dirty_cells(cxt, modified_cells);
    cout << "number of dirty cells: " << dirty_cells.size() << endl;

TBD

::

    // Now perform calculation.
    ixion::calculate_sorted_cells(cxt, dirty_cells, 0);

    double value = cxt.get_numeric_value(A11);
    cout << "value of A11: " << value << endl;


Modify formula cell
-------------------

TBD

::

    s = "AVERAGE(A1:A10)";
    tokens = ixion::parse_formula_string(cxt, A11, *resolver, s.data(), s.size());

    // Before overwriting, make sure to UN-register the old cell.
    ixion::unregister_formula_cell(cxt, A11);

    // Set and register the new formula cell.
    cell = cxt.set_formula_cell(A11, std::move(tokens));
    ixion::register_formula_cell(cxt, A11, cell);

TBD

::

    // This time, we know that none of the cell values have changed, but the
    // formula A11 is updated & needs recalculation.
    ixion::abs_range_set_t modified_formula_cells{A11};
    dirty_cells = ixion::query_and_sort_dirty_cells(cxt, ixion::abs_range_set_t(), &modified_formula_cells);
    cout << "number of dirty cells: " << dirty_cells.size() << endl;

    // Perform calculation again.
    ixion::calculate_sorted_cells(cxt, dirty_cells, 0);

    value = cxt.get_numeric_value(A11);
    cout << "value of A11: " << value << endl;

TBD

