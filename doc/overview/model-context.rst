
.. highlight:: cpp

.. _use-model-context:

First step - creating model_context
===================================

Create a model context instance
-------------------------------

When using ixion, the very first step is to create a :cpp:class:`~ixion::model_context`
instance:

.. literalinclude:: ../../doc_example/model_context_simple.cpp
   :language: C++
   :start-after: //!code-start: instantiate
   :end-before: //!code-end: instantiate
   :dedent: 4

The :cpp:class:`~ixion::model_context` class represents a document model data
store that stores cell values spreading over one or more sheets.  At the time of construction,
the model contains no sheets. So the obvious next step is to insert a sheet:

.. literalinclude:: ../../doc_example/model_context_simple.cpp
   :language: C++
   :start-after: //!code-start: insert-sheet
   :end-before: //!code-end: insert-sheet
   :dedent: 4

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

.. literalinclude:: ../../doc_example/model_context_simple.cpp
   :language: C++
   :start-after: //!code-start: set-numeric-cell
   :end-before: //!code-end: set-numeric-cell
   :dedent: 4

Note that, since row and column positions are internally 0-based, we add one to emulate how the row
positions are presented in typical spreadsheet program.

Inserting a string value can be done via :cpp:func:`~ixion::model_context::set_string_cell` in one
of two ways.  The first way is to store the value to a type that decays to ``std::string_view``, such
as ``std::string``, char array, or string literal, and pass it to the method directly:

.. literalinclude:: ../../doc_example/model_context_simple.cpp
   :language: C++
   :start-after: //!code-start: set-string-cell-value
   :end-before: //!code-end: set-string-cell-value
   :dedent: 4

The second way is to add your string to the model_context's internal string pool first which will return its
string ID, and pass that ID to the method:

.. literalinclude:: ../../doc_example/model_context_simple.cpp
   :language: C++
   :start-after: //!code-start: set-string-cell-id
   :end-before: //!code-end: set-string-cell-id
   :dedent: 4

The model_context class has two methods for inserting a string to the string pool:
:cpp:func:`~ixion::model_context::add_string` and :cpp:func:`~ixion::model_context::append_string`.  The
:cpp:func:`~ixion::model_context::add_string` method checks for an existing entry with the same string value
upon each insertion attempt, and it will not insert the new value if the value already exists in the pool.
The :cpp:func:`~ixion::model_context::append_string` method, on the other hand, does not check the pool for
an existing value and always inserts the value.  The :cpp:func:`~ixion::model_context::append_string` method
is appropriate if you know all your string entries ahead of time and wish to bulk-insert them.  Otherwise
using :cpp:func:`~ixion::model_context::add_string` is appropriate in most cases.


Insert a formula cell into model context
----------------------------------------

Inserting a formula cell requires a few extra steps.  First, you need to tokenize your formula string, and
to do that, you need to create an instance of :cpp:class:`~ixion::formula_name_resolver`.  The
formula_name_resolver class is responsible for resolving "names" into references, functions, and named
expressions names.  Ixion provides multiple types of name resolvers, and you specify its type when passing
an enum value of type :cpp:enum:`~ixion::formula_name_resolver_t` when calling its static
:cpp:func:`ixion::formula_name_resolver::get` function.  In this example, we'll be using the Excel A1
syntax:

.. literalinclude:: ../../doc_example/model_context_simple.cpp
   :language: C++
   :start-after: //!code-start: resolver
   :end-before: //!code-end: resolver
   :dedent: 4

You can also optionally pass a memory address of your :cpp:class:`~ixion::model_context` instance which is
required for resolving sheet names.  You can pass a ``nullptr`` if you don't need to resolve sheet names.

Next, let's create a formula string we want to tokenize.  Here, we are inserting a formula expression
**SUM(A1:A10)** into cell A11:

.. literalinclude:: ../../doc_example/model_context_simple.cpp
   :language: C++
   :start-after: //!code-start: parse-formula-string
   :end-before: //!code-end: parse-formula-string
   :dedent: 4

To tokenize a formula string, you call the :cpp:func:`ixion::parse_formula_string` function and pass

* a model_context instance
* the position of the cell to insert the formula into,
* a formula_name_resolver instance, and
* the formula string to tokenize.

The function will then return a sequence of tokens representing the original formula string.  Once you
have the tokens, you can finally pass them to your model_context instance via
:cpp:func:`~ixion::model_context::set_formula_cell`:

.. literalinclude:: ../../doc_example/model_context_simple.cpp
   :language: C++
   :start-after: //!code-start: set-formula-tokens
   :end-before: //!code-end: set-formula-tokens
   :dedent: 4

There is a few things to note. First, you need to *move* your tokens to the method since instances of
type :cpp:type:`ixion::formula_tokens_t` are non-copyable and only movable.  Second, the method returns
a pointer to the formula cell instance that just got inserted into the model. We are saving it here
to use it in the next step below.

When inserting a formula cell, you need to "register" it so that the model can record its reference
dependencies via :cpp:func:`~ixion::register_formula_cell`:

.. literalinclude:: ../../doc_example/model_context_simple.cpp
   :language: C++
   :start-after: //!code-start: register-formula-cell
   :end-before: //!code-end: register-formula-cell
   :dedent: 4

Without registering formula cells, you won't be able to determine which formula cells to re-calculate
for given modified cells.  Here we are passing the pointer to the formula cell returned from the previous
call.  This is optional, and you can pass a ``nullptr`` instead. But by passing it you will avoid the
overhead of searching for the cell instance from the model.


Calculate formula cell
----------------------

Now that we have the formula cell in, let's run our first calculation.  To calcualte formula cells, you
need to first specify a range of modified cells in order to query for all formula cells affected by it
either directly or indirectly, which we refer to as "dirty" formula cells.  Since this is our initial
calculation, we can simply specify the entire sheet to be "modified" which will effectively trigger all
formula cells:

.. literalinclude:: ../../doc_example/model_context_simple.cpp
   :language: C++
   :start-after: //!code-start: dirty-sheet
   :end-before: //!code-end: dirty-sheet
   :dedent: 4

We will then pass it to :cpp:func:`~ixion::query_and_sort_dirty_cells` to get a sequence of formula cell
addresses to calculate:

.. literalinclude:: ../../doc_example/model_context_simple.cpp
   :language: C++
   :start-after: //!code-start: sort-dirty-cells
   :end-before: //!code-end: sort-dirty-cells
   :dedent: 4

Since so far we only have one formula cell, this should only return one range with the size of one row and one column.  You
will see the following output:

.. code-block:: text

    number of dirty cells: 1

Let's inspect which cell it actually refers to:

.. literalinclude:: ../../doc_example/model_context_simple.cpp
   :language: C++
   :start-after: //!code-start: cout-dirty-cell
   :end-before: //!code-end: cout-dirty-cell
   :dedent: 4

which will print:

.. code-block:: text

    dirty cell: (sheet:0; row:10; column:0)-(sheet:0; row:10; column:0)

confirming that it certainly points to cell A11.  Finally, pass this to :cpp:func:`~ixion::calculate_sorted_cells`:

.. literalinclude:: ../../doc_example/model_context_simple.cpp
   :language: C++
   :start-after: //!code-start: calculate
   :end-before: //!code-end: calculate
   :dedent: 4

to calculate cell A11.  After that, you can retrieve the result of the calculation by calling
:cpp:func:`~ixion::model_context::get_numeric_value` for A11:

.. literalinclude:: ../../doc_example/model_context_simple.cpp
   :language: C++
   :start-after: //!code-start: print-value
   :end-before: //!code-end: print-value
   :dedent: 4

You will see the following output:

.. code-block:: text

    value of A11: 55


Modify formula cell
-------------------

Let's say you need to overwrite the formula in A11 to something else.  The steps you need to take
are very similar to the steps for inserting a brand-new formula cell, the only difference being
that you need to "unregister" the old formula cell before overwriting it.

Let's go through this step by step.  First, create new tokens to insert:

.. literalinclude:: ../../doc_example/model_context_simple.cpp
   :language: C++
   :start-after: //!code-start: parse-formula-a11
   :end-before: //!code-end: parse-formula-a11
   :dedent: 4

This time we are inserting the formula **AVERAGE(A1:A10)** in A11 to overwrite the previous one
**SUM(A1:A10)**.  Before inserting these tokens, first unregister the current formula cell:

.. literalinclude:: ../../doc_example/model_context_simple.cpp
   :language: C++
   :start-after: //!code-start: unregister-a11
   :end-before: //!code-end: unregister-a11
   :dedent: 4

This will remove the dependency information of the old formula from the model's internal tracker.
Once that's done, the rest is the same as inserting a new formula:

.. literalinclude:: ../../doc_example/model_context_simple.cpp
   :language: C++
   :start-after: //!code-start: update-formula-a11
   :end-before: //!code-end: update-formula-a11
   :dedent: 4

Let's re-calculate the new formula cell.  The re-calculation steps are also very similar to the initial
calculation steps.  The first step is to query for all dirty formula cells.  This time, however, we don't
query based on which formula cells are affected by modified cells, which we'll specify as none.  Instead,
we query based on which formula cells have been modified, which in this case is A11:

.. literalinclude:: ../../doc_example/model_context_simple.cpp
   :language: C++
   :start-after: //!code-start: sort-a11
   :end-before: //!code-end: sort-a11
   :dedent: 4

As is the first calculation, you should only get one dirty cell address from the :cpp:func:`~ixion::query_and_sort_dirty_cells`
call.  Running the above code should produce:

.. code-block:: text

    number of dirty cells: 1

The rest should be familiar:

.. literalinclude:: ../../doc_example/model_context_simple.cpp
   :language: C++
   :start-after: //!code-start: recalc-a11
   :end-before: //!code-end: recalc-a11
   :dedent: 4

You should see the following output when finished:

.. code-block:: text

    value of A11: 5.5


Formula cell with no references
-------------------------------

Next example shows a scenario where you want to overwrite a cell in A10, which
currently stores a numeric value, with a formula cell that references no other
cells.  Let's add the new formula cell first:

.. literalinclude:: ../../doc_example/model_context_simple.cpp
   :language: C++
   :start-after: //!code-start: overwrite-a10
   :end-before: //!code-end: overwrite-a10
   :dedent: 4

Here, we are not registering this cell since it contains no references hence it
does not need to be tracked by dependency tracker.  Also, since the previous
cell in A10 is not a formula cell, there is no cell to unregister.

.. warning::

    Technically speaking, every formula cell that contains references to other
    cells or contains at least one volatile function needs to be registered.
    Since registering a formula cell that doesn't need to be registered is
    entirely harmless (albeit a slight overhead), it's generally a good idea to
    register every new formula cell regardless of its content.

    Likewise, unregistering a formula cell that didn't need to be registered
    (or wasn't registered) is entirely harmless.  Even unregistering a cell
    that didn't contain a formula cell is harmless, and essentially does
    nothing.  As such, it's probably a good idea to unregister a cell whenever
    a new cell value is being placed.

Let's obtain all formula cells in need to re-calculation:

.. literalinclude:: ../../doc_example/model_context_simple.cpp
   :language: C++
   :start-after: //!code-start: a10-dirty-cells
   :end-before: //!code-end: a10-dirty-cells
   :dedent: 4

Here, we are only passing one modified formula cell which is A10, and no other
cells being modified.  Since cell A11 references ``A1:A10`` and A10's value has
changed, this should also trigger A11 for re-calculation.  Running this code
should produce the following output:

.. code-block:: text

    number of dirty cells: 2

Let's calculate all affected formula cells and check the results of A10 and A11:

.. literalinclude:: ../../doc_example/model_context_simple.cpp
   :language: C++
   :start-after: //!code-start: a10-dirty-cells-calc
   :end-before: //!code-end: a10-dirty-cells-calc
   :dedent: 4

Running this code should produce the following output:

.. code-block:: text

    value of A10: 75
    value of A11: 12

The complete source code of this example is avaiable `here <https://gitlab.com/ixion/ixion/-/blob/master/doc_example/model_context_simple.cpp>`_.

