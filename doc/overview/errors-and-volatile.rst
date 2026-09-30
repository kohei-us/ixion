.. highlight:: cpp

.. _errors-and-volatile:

Errors and volatile functions
=============================

Two things come up in every model sooner or later: a formula that can't
produce a value, and a formula whose value changes on its own.  This page
covers how Ixion represents error values and how they travel through a
model, what makes a formula cell *volatile*, and what you get when you
read a formula cell before it has been calculated.

We'll reuse the ``set_formula()`` and ``calculate()`` helpers from the
earlier pages, with the modified cells as a parameter of ``calculate()``
this time:

.. literalinclude:: ../../doc_example/errors_and_volatile.cpp
   :language: C++
   :start-after: //!code-start: helpers
   :end-before: //!code-end: helpers

And one more helper that prints the result of a formula cell along with
its type:

.. literalinclude:: ../../doc_example/errors_and_volatile.cpp
   :language: C++
   :start-after: //!code-start: print-result
   :end-before: //!code-end: print-result


Error values
------------

An error is one of the things a :cpp:class:`~ixion::formula_result` can
hold, next to a number, a boolean, a string or a matrix.  The kinds of
error are the values of :cpp:enum:`~ixion::formula_error_t`, and each has
the name you'd see in a spreadsheet cell:

.. list-table::
   :header-rows: 1
   :widths: 30 15 55

   * - Value
     - Name
     - Meaning
   * - ``ref_result_not_available``
     - ``#REF!``
     - A referenced result is not available.  This is also what the cells
       of a circular reference end up with.
   * - ``division_by_zero``
     - ``#DIV/0!``
     - Division by zero.
   * - ``invalid_expression``
     - ``#NUM!``
     - The expression could not be evaluated.
   * - ``name_not_found``
     - ``#NAME?``
     - A function or named expression is unknown.
   * - ``no_range_intersection``
     - ``#NULL!``
     - Two ranges were expected to intersect but don't.
   * - ``invalid_value_type``
     - ``#VALUE!``
     - A value of the wrong type was used, such as a string where a number
       was needed.
   * - ``no_value_available``
     - ``#N/A``
     - No value is available, as produced by ``NA()``.

Let's produce one.  C1 divides A1 by B1, which is zero, D1 adds one to
C1, and E1 asks whether C1 is an error:

.. literalinclude:: ../../doc_example/errors_and_volatile.cpp
   :language: C++
   :start-after: //!code-start: setup
   :end-before: //!code-end: setup
   :dedent: 4

.. code-block:: text

    C1: #DIV/0! (error)
    D1: #DIV/0! (error)
    E1: true (boolean)

C1 gets the division error, and D1 gets the same error, because an error
used as an operand becomes the result of the operation.  That's how an
error spreads to everything that depends on the cell, until a formula
handles it explicitly, as ``ISERROR()`` does in E1.  An error is a regular
result in every other respect: it's cached in the cell, it takes part in
dependency tracking like any value, and calculation carries on with the
rest of the model.

To find out which error a cell holds, check the type of its result and ask
for the error value.  :cpp:func:`~ixion::get_formula_error_name` gives you
the display name, and :cpp:func:`~ixion::to_formula_error_type` goes the
other way:

.. literalinclude:: ../../doc_example/errors_and_volatile.cpp
   :language: C++
   :start-after: //!code-start: error-type
   :end-before: //!code-end: error-type
   :dedent: 4

.. code-block:: text

    C1 error: #DIV/0!
    C1 is division by zero: true

The typed accessors can't return an error, so asking for the numeric value
of a cell in error throws a :cpp:class:`~ixion::formula_error` instead,
carrying the same error value:

.. literalinclude:: ../../doc_example/errors_and_volatile.cpp
   :language: C++
   :start-after: //!code-start: numeric-throws
   :end-before: //!code-end: numeric-throws
   :dedent: 4

.. code-block:: text

    C1 cannot be read as a number: #DIV/0!

If you don't know what type of value to expect from a cell, go through
:cpp:func:`~ixion::model_context::get_formula_result` and check the type
first, as ``print_result()`` does.

Errors clear the same way they appear.  Once B1 holds a usable value, the
next calculation replaces the errors downstream:

.. literalinclude:: ../../doc_example/errors_and_volatile.cpp
   :language: C++
   :start-after: //!code-start: fix-error
   :end-before: //!code-end: fix-error
   :dedent: 4

.. code-block:: text

    C1: 2.5 (value)
    D1: 3.5 (value)
    E1: false (boolean)

A circular reference is the one error the calculation itself produces.
When formula cells depend on each other in a cycle, none of them can be
calculated first, so all of them get ``#REF!``, and the cells outside the
cycle are calculated as usual:

.. literalinclude:: ../../doc_example/errors_and_volatile.cpp
   :language: C++
   :start-after: //!code-start: circular
   :end-before: //!code-end: circular
   :dedent: 4

.. code-block:: text

    A3: #REF! (error)
    B3: #REF! (error)
    C3: 20 (value)


Volatile functions
------------------

Most formula cells only need re-calculating when something they reference
has changed.  A formula that uses ``NOW()``, ``TODAY()`` or ``RAND()`` is
different: its value changes with no change to any cell.  Such a function
is *volatile*, and so is any formula cell that uses one.

Ixion handles this in the dirty query.  A volatile formula cell is
reported dirty on every call to
:cpp:func:`~ixion::query_and_sort_dirty_cells`, whether or not anything
was modified, and so are the cells that depend on it:

.. literalinclude:: ../../doc_example/errors_and_volatile.cpp
   :language: C++
   :start-after: //!code-start: volatile
   :end-before: //!code-end: volatile
   :dedent: 4

.. code-block:: text

    dirty cells with nothing modified: 2
    A5 changed: true
    B5 follows A5: true

The two dirty cells are A5 and its dependent B5.  Volatility is recorded
when the cell is set, at the same time as its references, and from then on
the cell comes back dirty on every calculation.


Results that aren't there yet
-----------------------------

A formula cell you've just inserted has no result until it's calculated,
and neither has a cell whose calculation is still in progress on another
thread.  What happens when you read such a cell depends on the model's
*wait policy*, a :cpp:enum:`~ixion::formula_result_wait_policy_t` value you
can inspect with
:cpp:func:`~ixion::model_context::get_formula_result_wait_policy`.

The model context manages the policy itself.  While
:cpp:func:`~ixion::calculate_sorted_cells` is running, the policy is to
block: a formula cell that needs a result still being computed on another
thread waits for it.  Outside of a calculation, the policy is to throw,
and reading a cell without a result throws a
:cpp:class:`~ixion::formula_error` with the ``#REF!`` error value:

.. literalinclude:: ../../doc_example/errors_and_volatile.cpp
   :language: C++
   :start-after: //!code-start: not-yet
   :end-before: //!code-end: not-yet
   :dedent: 4

.. code-block:: text

    A7 not calculated yet: #REF!
    A7 value: 11

The complete source code of this example is available
`here <https://gitlab.com/ixion/ixion/-/blob/master/doc_example/errors_and_volatile.cpp>`_.
