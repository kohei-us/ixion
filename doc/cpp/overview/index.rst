
.. highlight:: cpp

Overview
========

Create a model context instance
-------------------------------

When using ixion, the very first step is to create a :cpp:class:`~ixion::model_context`
instance::

    ixion::model_context cxt;

The :cpp:class:`~ixion::model_context` class represents a document model data
store that stores cell values spanning over one or more sheets.  At the time of construction,
the model contains no sheets. So the obvious next step is to insert at least one sheet::

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

TBD

::

    // Now, populate it with some numeric values in A1:A10.
    for (ixion::abs_address_t pos(0, 0, 0); pos.row <= 9; ++pos.row)
    {
        double value = pos.row + 1.0; // Set the row position + 1 as the cell value.
        cxt.set_numeric_cell(pos, value);
    }

TBD

::

    // Insert a string value into B2.
    ixion::abs_address_t B2(0, 1, 1);
    std::string s = "This cell contains a string value.";
    cxt.set_string_cell(B2, s.data(), s.size());

TBD

::

    // Insert a literal string value into B3.
    ixion::abs_address_t B3(0, 2, 1);
    cxt.set_string_cell(B3, IXION_ASCII("This too contains a string value."));

TBD

::

    // Insert a string value into B4 via string identifier.
    s = "Yet another string value.";
    ixion::string_id_t sid = cxt.add_string(s.data(), s.size());
    ixion::abs_address_t B4(0, 3, 1);
    cxt.set_string_cell(B4, sid);


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

