
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
.. doxygenenum:: ixion::formula_result_wait_policy_t
.. doxygenenum:: ixion::formula_event_t
.. doxygenenum:: ixion::rc_direction_t

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

.. doxygenstruct:: ixion::rc_size_t
.. doxygenstruct:: ixion::formula_group_t


Cell Addresses
--------------

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

.. doxygenstruct:: ixion::rc_range_t
   :members:

.. doxygenstruct:: ixion::abs_range_t
   :members:

.. doxygenstruct:: ixion::abs_rc_range_t
   :members:

.. doxygenstruct:: ixion::table_t
   :members:

.. doxygentypedef:: ixion::abs_address_set_t

.. doxygentypedef:: ixion::abs_range_set_t

.. doxygentypedef:: ixion::abs_rc_range_set_t


Column Blocks
-------------

.. doxygentypedef:: ixion::column_block_handle
.. doxygentypedef:: ixion::column_block_callback_t
.. doxygenenum:: ixion::column_block_t

.. doxygenstruct:: ixion::column_block_shape_t
   :members:

Utility Functions
-----------------

.. doxygenfunction:: ixion::get_formula_error_name

.. doxygenfunction:: ixion::to_formula_error_type
