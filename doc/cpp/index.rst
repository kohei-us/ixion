.. highlight:: cpp

C++ API
=======

Formula API
-----------

.. doxygenfunction:: ixion::parse_formula_string

.. doxygenfunction:: ixion::print_formula_tokens

.. doxygenfunction:: ixion::register_formula_cell

.. doxygenfunction:: ixion::unregister_formula_cell

.. doxygenfunction:: ixion::get_all_dirty_cells

.. doxygenfunction:: ixion::calculate_cells


Primitive Types
---------------

.. doxygentypedef:: ixion::col_t
.. doxygentypedef:: ixion::row_t
.. doxygentypedef:: ixion::sheet_t
.. doxygentypedef:: ixion::string_id_t

.. doxygenvariable:: ixion::global_scope
.. doxygenvariable:: ixion::invalid_sheet
.. doxygenvariable:: ixion::empty_string_id

.. doxygenenum:: ixion::celltype_t
.. doxygenenum:: ixion::value_t

.. doxygenclass:: ixion::values_t

.. doxygenenum:: ixion::table_area_t

.. doxygentypedef:: ixion::table_areas_t

.. doxygenenum:: ixion::formula_name_resolver_t

.. doxygentypedef:: ixion::column_store_t
.. doxygentypedef:: ixion::column_stores_t

.. doxygenenum:: ixion::formula_error_t

.. doxygenfunction:: ixion::get_formula_error_name


Address Types
-------------

.. doxygenstruct:: ixion::address_t
   :members:

.. doxygenstruct:: ixion::abs_address_t
   :members:

.. doxygenstruct:: ixion::range_t
   :members:

.. doxygenstruct:: ixion::abs_range_t
   :members:

.. doxygentypedef:: ixion::dirty_formula_cells_t

.. doxygentypedef:: ixion::modified_cells_t


Formula Name Resolver
---------------------

.. doxygenclass:: ixion::formula_name_resolver
   :members:

.. doxygenstruct:: ixion::formula_name_t
   :members:

.. doxygenfunction:: ixion::to_address

.. doxygenfunction:: ixion::to_range


Model Context
-------------

.. doxygenclass:: ixion::model_context
   :members:


Formula Cell
------------

.. doxygenclass:: ixion::formula_cell
   :members:

.. doxygenclass:: ixion::formula_result
   :members:


Interfaces
----------

.. doxygenclass:: ixion::iface::formula_model_access
   :members:

.. doxygenclass:: ixion::iface::session_handler
   :members:

.. doxygenclass:: ixion::iface::table_handler
   :members:

