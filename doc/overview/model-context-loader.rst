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
:cpp:class:`~ixion::model_context` optimized for bulk loading.  It wraps a
model you own for the duration of the load, its setters only write, and the
validation and the recording of references happen once, in
:cpp:func:`~ixion::model_context_loader::finalize`.  After that the loader
has done its job and you can discard it.  Validating everything at the end
also loosens the order the content has to come in: you can load the cells
first and the rest, such as named expressions and tables, afterwards, or the
other way around.


Create a loader
---------------

You create the model and a name resolver on it as usual, then hand the
model to the loader.  From here on you talk to the loader, starting with the
sheets:

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


Finalize
--------

Once everything is in, call
:cpp:func:`~ixion::model_context_loader::finalize`.  It validates every
formula cell set through the loader and records their references with the
dirty cell tracker:

.. literalinclude:: ../../doc_example/model_context_loader.cpp
   :language: C++
   :start-after: //!code-start: finalize
   :end-before: //!code-end: finalize
   :dedent: 4

This ends the load: any further call on the loader throws, and you can let
it go.  The model is yours to use as usual.  The results you loaded are
readable right away, and the references are in place, so a change to a cell
propagates the usual way:

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

* You leave the cells alone while the loader works on the model: don't
  modify or calculate them through :cpp:class:`~ixion::model_context` until
  :cpp:func:`~ixion::model_context_loader::finalize` has run.

The complete source code of this example is available
`here <https://gitlab.com/ixion/ixion/-/blob/master/doc_example/model_context_loader.cpp>`_.
