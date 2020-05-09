
Types
=====

Macros
------

.. doxygendefine:: IXION_ASCII


Primitive Types
---------------

.. doxygenenum:: ixion::celltype_t
.. doxygenenum:: ixion::cell_value_t
.. doxygenenum:: ixion::value_t
.. doxygenenum:: ixion::table_area_t
.. doxygenenum:: ixion::formula_name_resolver_t
.. doxygenenum:: ixion::formula_error_t

.. doxygentypedef:: ixion::col_t
.. doxygentypedef:: ixion::row_t
.. doxygentypedef:: ixion::sheet_t
.. doxygentypedef:: ixion::rc_t
.. doxygentypedef:: ixion::string_id_t
.. doxygentypedef:: ixion::table_areas_t
.. doxygentypedef:: ixion::formula_tokens_t

.. doxygenvariable:: ixion::empty_string_id
.. doxygenvariable:: ixion::global_scope
.. doxygenvariable:: ixion::invalid_sheet

.. doxygenfunction:: ixion::get_formula_error_name


Column Store Types
------------------

.. doxygentypedef:: ixion::boolean_element_block
.. doxygentypedef:: ixion::numeric_element_block
.. doxygentypedef:: ixion::string_element_block
.. doxygentypedef:: ixion::formula_element_block
.. doxygentypedef:: ixion::ixion_element_block_func
.. doxygentypedef:: ixion::column_store_t
.. doxygentypedef:: ixion::column_stores_t
.. doxygentypedef:: ixion::matrix_store_t

.. doxygenstruct:: ixion::matrix_store_trait

.. doxygenvariable:: ixion::element_type_empty
.. doxygenvariable:: ixion::element_type_boolean
.. doxygenvariable:: ixion::element_type_numeric
.. doxygenvariable:: ixion::element_type_string
.. doxygenvariable:: ixion::element_type_formula


Address Types
-------------

.. doxygenstruct:: ixion::address_t
   :members:

.. doxygenstruct:: ixion::rc_address_t
   :members:

.. doxygenstruct:: ixion::abs_address_t
   :members:

.. doxygenstruct:: ixion::abs_rc_address_t
   :members:

.. doxygenstruct:: ixion::range_t
   :members:

.. doxygenstruct:: ixion::abs_range_t
   :members:

.. doxygenstruct:: ixion::abs_rc_range_t
   :members:

.. doxygentypedef:: ixion::abs_address_set_t

.. doxygentypedef:: ixion::abs_range_set_t

.. doxygentypedef:: ixion::abs_rc_range_set_t
