.. highlight:: cpp

.. _model-context-loader:

Loading a model in bulk
=======================

Sometimes you build a whole model in one go from content that already
exists, rather than editing cells one at a time: an importer feeding a
document into ixion, say, or a program rebuilding a model it took apart
earlier.  The volume is large, so the work each
:cpp:class:`~ixion::model_context` setter does on every call adds up: it
checks what it replaces, validates a formula and records its references
right away.

:cpp:class:`~ixion::model_context_loader` is a stand-in for
:cpp:class:`~ixion::model_context` optimized for bulk loading.  It creates
and owns a new model, its setters only write, and the validation and the
recording of references happen once, in
:cpp:func:`~ixion::model_context_loader::finalize`, which then hands the
finished model to you.  Validating everything at the end also loosens the
order the content has to come in: you can load the cells first and the rest,
such as named expressions and tables, afterwards, or the other way around.


Create a loader
---------------

A loader gets constructed the way a :cpp:class:`~ixion::model_context` does,
with the default sheet size or a custom one.  Since the model lives inside
the loader, you append the sheets and create the name resolver through the
loader too:

.. literalinclude:: ../../doc_example/model_context_loader.cpp
   :language: C++
   :start-after: //!code-start: create-loader
   :end-before: //!code-end: create-loader
   :dedent: 4

The loader offers the setters you know from
:cpp:class:`~ixion::model_context`, minus the ones that overwrite or empty
cells, since a load only ever fills empty cells.  Let's put some values in:

.. literalinclude:: ../../doc_example/model_context_loader.cpp
   :language: C++
   :start-after: //!code-start: values
   :end-before: //!code-end: values
   :dedent: 4


Formula cells and a name
------------------------

Now the formula cells.  Content loaded in bulk usually brings its results
along, so each formula gets one as a :cpp:class:`~ixion::formula_result`.
The first formula uses a name, ``MyData``, which we define right after:

.. literalinclude:: ../../doc_example/model_context_loader.cpp
   :language: C++
   :start-after: //!code-start: formulas
   :end-before: //!code-end: formulas
   :dedent: 4

Here is the name, set through the same
:cpp:func:`~ixion::model_context_loader::set_named_expression` you would
call on the model:

.. literalinclude:: ../../doc_example/model_context_loader.cpp
   :language: C++
   :start-after: //!code-start: name
   :end-before: //!code-end: name
   :dedent: 4


Finalize and take the model
---------------------------

Once everything is in, call
:cpp:func:`~ixion::model_context_loader::finalize`.  It validates every
formula cell set through the loader, records their references with the
dirty cell tracker, and returns the model:

.. literalinclude:: ../../doc_example/model_context_loader.cpp
   :language: C++
   :start-after: //!code-start: finalize
   :end-before: //!code-end: finalize
   :dedent: 4

This ends the load.  The loader has nothing left in it, and any further call
on it throws.  The name resolver you created through the loader points at
the model that just moved out, so it's no good any more either; create a new
one on the returned model if you need to parse more formulas.

.. warning::

    A name resolver created through the loader becomes invalid once
    :cpp:func:`~ixion::model_context_loader::finalize` has returned the
    model.  Create a new one on the returned model instead.

From here on, the model is an ordinary :cpp:class:`~ixion::model_context`.
The results you loaded are readable right away, and the references are in
place, so a change to a cell propagates the usual way:

.. literalinclude:: ../../doc_example/model_context_loader.cpp
   :language: C++
   :start-after: //!code-start: use-model
   :end-before: //!code-end: use-model
   :dedent: 4

.. code-block:: text

    C1 = 66
    C2 = 11
    C1 = 165
    C2 = 110


What the loader leaves to you
-----------------------------

The loader's setters only write, without the extra validations the regular
:cpp:class:`~ixion::model_context` performs, so you as the user have a few
extra responsibilities:

* You write each cell at most once.  A setter aimed at a cell that isn't
  empty throws an exception.

* You append every sheet a formula references before you set the formula,
  because the parser resolves sheet names against the sheets already present
  in the model.

* You define every named expression and table a formula references before
  you call :cpp:func:`~ixion::model_context_loader::finalize`, since that
  is where the references get resolved and dependencies get recorded.

The complete source code of this example is available
`here <https://gitlab.com/ixion/ixion/-/blob/master/doc_example/model_context_loader.cpp>`_.
