
.. highlight:: cpp

.. _use-document:

Use document class
==================

In the :ref:`use-model-context` section, we saw an example of how to
set up a cell value store and run some simple calculations using the
:cpp:class:`~ixion::model_context` class.  While that approach certainly works
fine, one large drawback is that you do need to manually handle formula tokenization,
formula cell registration (and un-registration), as well as to trace which cells
have their values changed and which formula cells have been created or modified.
This is because the :cpp:class:`~ixion::model_context` class is designed to only
handle cell value storage, and all other operations related to formula expressions
and formula cell (re-)calculations have to be done outside of it.

Luckily, Ixion also provides a higher level document class called
:cpp:class:`~ixion::document` which internally uses :cpp:class:`~ixion::model_context`
and handles all the formula cell related operations internally.  This section
provides an overview of how to use the :cpp:class:`~ixion::document` class to
do more or less similar things we did in the :ref:`use-model-context`
section.

First, we need to instantiate the :cpp:class:`~ixion::document` instance and
insert a sheet named ``MySheet``.

::

    ixion::document doc;
    doc.append_sheet("MySheet");

Next, like we did in the previous section, we will insert numbers 1 through 10
in cells A1 through A10::

    for (ixion::abs_address_t pos(0, 0, 0); pos.row <= 9; ++pos.row)
    {
        double value = pos.row + 1.0; // Set the row position + 1 as the cell value.
        doc.set_numeric_cell(pos, value);
    }

So far we don't see much of a difference from model_context.  Let's now insert
string values into cells B2 and B3::

    // Insert string values.
    std::string s = "This cell contains a string value.";
    doc.set_string_cell("B2", s);
    doc.set_string_cell("B3", "This too contains a string value.");

Here we see the first difference.  When using :cpp:class:`~ixion::document`,
You can specify the cell position either by :cpp:struct:`~ixion::abs_address_t`
as with :cpp:class:`~ixion::model_context`, or by a string whose value is the
name of the cell address.  The default address syntax for the string cell address
is "Excel A1" syntax.  You can pick a different syntax by passing a value of type
:cpp:enum:`~ixion::formula_name_resolver_t` to the constructor.

It's worth noting that, when specifying the cell position as a string value and
the sheet name is omitted, the first sheet is implied.  You can also specify
the sheet name explicitly as in the following::

    doc.set_string_cell("MySheet!B4", "Yet another string value.");

For a document with only one sheet, it makes no difference whether to include
the sheet name or leave it out, but if you have more than one sheet, you need to
specify the sheet name when specifying a cell position on sheets other than the
first one.

Now, let's insert a a formula into A11 to sum up values in A1:A10, and calculate
it afterward::

    doc.set_formula_cell("A11", "SUM(A1:A10)");
    doc.calculate(0);

And fetch the calculated value in A11 and see what the result is::

    double value = doc.get_numeric_value("A11");
    cout << "value of A11: " << value << endl;

You should see the following output:

.. code-block:: text

    value of A11: 55

It looks about right.  The :cpp:func:`~ixion::document::calculate` method takes one
argument that is the number of threads to use for the calculation.  We pass 0 here to
run the calculation using only the main thread.

Now, let's re-write the formula in cell A11 to take the average of A1:A10 instead,
run the calculation again, and check the value of A11::

    // Insert a new formula to A11.
    doc.set_formula_cell("A11", "AVERAGE(A1:A10)");
    doc.calculate(0);

    value = doc.get_numeric_value("A11");
    cout << "value of A11: " << value << endl;

The output says:

.. code-block:: text

    value of A11: 5.5

which looks right.  Note that, unlike the previous example, there is no need to un-register
and register cell A11 before and after the edit.

Lastly, let's insert into cell A10 a new formula that contains no references to other cells.
As this will trigger a re-calculation of cell A11, we will check the values of both A10
and A11::

    // Overwrite A10 with a formula cell with no references.
    doc.set_formula_cell("A10", "(100+50)/2");
    doc.calculate(0);

    value = doc.get_numeric_value("A10");
    cout << "value of A10: " << value << endl;

    value = doc.get_numeric_value("A11");
    cout << "value of A11: " << value << endl;

The output will be:

.. code-block:: text

    value of A10: 75
    value of A11: 12

Notice once again that there is no need to do formula cell registration nor manual tracking
of dirty formula cells.


Conclusion
----------

In this section, we have performed the same thing we did in the :ref:`use-model-context`
section, but with much less code, and without the complexity of low-level formula expression
tokenization, formula cell registration, or manual tracking of modified cells.  If you are
looking to leverage the functionality of Ixion but don't want to deal with lower-level formula
API, using the :cpp:class:`~ixion::document` class may be just the ticket.

The complete source code of this example is avaiable `here <https://gitlab.com/ixion/ixion/-/blob/master/doc_example/document_simple.cpp>`_.
