
.. highlight:: cpp

.. _use-document:

Using document for builtin formula management
=============================================

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

.. literalinclude:: ../../doc_example/document_simple.cpp
   :language: C++
   :start-after: //!code-start: instantiate
   :end-before: //!code-end: instantiate
   :dedent: 4

Next, like we did in the previous section, we will insert numbers 1 through 10
in cells A1 through A10:

.. literalinclude:: ../../doc_example/document_simple.cpp
   :language: C++
   :start-after: //!code-start: set-values
   :end-before: //!code-end: set-values
   :dedent: 4

So far we don't see much of a difference from model_context.  Let's now insert
string values into cells B2 and B3:

.. literalinclude:: ../../doc_example/document_simple.cpp
   :language: C++
   :start-after: //!code-start: insert-strings-1
   :end-before: //!code-end: insert-strings-1
   :dedent: 4

Here we see the first difference.  When using :cpp:class:`~ixion::document`,
You can specify the cell position either by :cpp:struct:`~ixion::abs_address_t`
as with :cpp:class:`~ixion::model_context`, or by a string whose value is the
name of the cell address.  The default address syntax for the string cell address
is "Excel A1" syntax.  You can pick a different syntax by passing a value of type
:cpp:enum:`~ixion::formula_name_resolver_t` to the constructor.

It's worth noting that, when specifying the cell position as a string value and
the sheet name is omitted, the first sheet is implied.  You can also specify
the sheet name explicitly as in the following:

.. literalinclude:: ../../doc_example/document_simple.cpp
   :language: C++
   :start-after: //!code-start: insert-string-2
   :end-before: //!code-end: insert-string-2
   :dedent: 4

For a document with only one sheet, it makes no difference whether to include
the sheet name or leave it out, but if you have more than one sheet, you need to
specify the sheet name when specifying a cell position on sheets other than the
first one.

Now, let's insert a formula into A11 to sum up values in A1:A10, and calculate
it afterward:

.. literalinclude:: ../../doc_example/document_simple.cpp
   :language: C++
   :start-after: //!code-start: set-formula-and-calc
   :end-before: //!code-end: set-formula-and-calc
   :dedent: 4

And fetch the calculated value in A11 and see what the result is:

.. literalinclude:: ../../doc_example/document_simple.cpp
   :language: C++
   :start-after: //!code-start: set-value-and-print
   :end-before: //!code-end: set-value-and-print
   :dedent: 4

You should see the following output:

.. code-block:: text

    value of A11: 55

It looks about right.  The :cpp:func:`~ixion::document::calculate` method takes one
argument that is the number of threads to use for the calculation.  We pass 0 here to
run the calculation using only the main thread.

Now, let's re-write the formula in cell A11 to take the average of A1:A10 instead,
run the calculation again, and check the value of A11:

.. literalinclude:: ../../doc_example/document_simple.cpp
   :language: C++
   :start-after: //!code-start: update-a11-and-recalc
   :end-before: //!code-end: update-a11-and-recalc
   :dedent: 4

The output says:

.. code-block:: text

    value of A11: 5.5

which looks right.  Note that, unlike the previous example, there is no need to un-register
and register cell A11 before and after the edit.

Lastly, let's insert into cell A10 a new formula that contains no references to other cells.
As this will trigger a re-calculation of cell A11, we will check the values of both A10
and A11:

.. literalinclude:: ../../doc_example/document_simple.cpp
   :language: C++
   :start-after: //!code-start: update-a10-and-recalc
   :end-before: //!code-end: update-a10-and-recalc
   :dedent: 4

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
