
Using cell_access for repeated operations on same cell
======================================================

You can obtain a :cpp:class:`~ixion::cell_access` instance either from
:cpp:class:`~ixion::model_context` or :cpp:class:`~ixion::document` class.

Here is an example of how to obtain it from a :cpp:class:`~ixion::model_context` instance:

.. literalinclude:: ../../doc_example/cell_access.cpp
   :language: C++
   :start-after: //!code-start: get-from-model-context
   :end-before: //!code-end: get-from-model-context
   :dedent: 4

Here is an example of how to obtain it from a :cpp:class:`~ixion::document` instance:

.. literalinclude:: ../../doc_example/cell_access.cpp
   :language: C++
   :start-after: //!code-start: get-from-document
   :end-before: //!code-end: get-from-document
   :dedent: 4

Once you have your :cpp:class:`~ixion::cell_access` instance, you can, for instance,
print the value of the cell as follows:

.. literalinclude:: ../../doc_example/cell_access.cpp
   :language: C++
   :start-after: //!code-start: get-value
   :end-before: //!code-end: get-value
   :dedent: 4

The complete source code of this example is avaiable
`here <https://gitlab.com/ixion/ixion/-/blob/master/doc_example/cell_access.cpp>`_.
