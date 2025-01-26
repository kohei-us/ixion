.. _ns-ixion:

namespace ixion
===============

Enum
----

cell_t
^^^^^^
.. doxygenenum:: ixion::cell_t

cell_value_t
^^^^^^^^^^^^
.. doxygenenum:: ixion::cell_value_t

column_block_t
^^^^^^^^^^^^^^
.. doxygenenum:: ixion::column_block_t

display_sheet_t
^^^^^^^^^^^^^^^
.. doxygenenum:: ixion::display_sheet_t

fopcode_t
^^^^^^^^^
.. doxygenenum:: ixion::fopcode_t

formula_error_t
^^^^^^^^^^^^^^^
.. doxygenenum:: ixion::formula_error_t

formula_event_t
^^^^^^^^^^^^^^^
.. doxygenenum:: ixion::formula_event_t

formula_function_t
^^^^^^^^^^^^^^^^^^
.. doxygenenum:: ixion::formula_function_t

formula_name_resolver_t
^^^^^^^^^^^^^^^^^^^^^^^
.. doxygenenum:: ixion::formula_name_resolver_t

formula_result_wait_policy_t
^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. doxygenenum:: ixion::formula_result_wait_policy_t

rc_direction_t
^^^^^^^^^^^^^^
.. doxygenenum:: ixion::rc_direction_t

table_area_t
^^^^^^^^^^^^
.. doxygenenum:: ixion::table_area_t

value_t
^^^^^^^
.. doxygenenum:: ixion::value_t


Type aliases
------------

abs_address_set_t
^^^^^^^^^^^^^^^^^
.. doxygentypedef:: ixion::abs_address_set_t

abs_range_set_t
^^^^^^^^^^^^^^^
.. doxygentypedef:: ixion::abs_range_set_t

abs_rc_range_set_t
^^^^^^^^^^^^^^^^^^
.. doxygentypedef:: ixion::abs_rc_range_set_t

calc_status_ptr_t
^^^^^^^^^^^^^^^^^
.. doxygentypedef:: ixion::calc_status_ptr_t

col_t
^^^^^
.. doxygentypedef:: ixion::col_t

column_block_callback_t
^^^^^^^^^^^^^^^^^^^^^^^
.. doxygentypedef:: ixion::column_block_callback_t

column_block_handle
^^^^^^^^^^^^^^^^^^^
.. doxygentypedef:: ixion::column_block_handle

formula_tokens_store_ptr_t
^^^^^^^^^^^^^^^^^^^^^^^^^^
.. doxygentypedef:: ixion::formula_tokens_store_ptr_t

formula_tokens_t
^^^^^^^^^^^^^^^^
.. doxygentypedef:: ixion::formula_tokens_t

rc_t
^^^^
.. doxygentypedef:: ixion::rc_t

row_t
^^^^^
.. doxygentypedef:: ixion::row_t

sheet_t
^^^^^^^
.. doxygentypedef:: ixion::sheet_t

string_id_t
^^^^^^^^^^^
.. doxygentypedef:: ixion::string_id_t

table_areas_t
^^^^^^^^^^^^^
.. doxygentypedef:: ixion::table_areas_t


Constants
---------

column_unset
^^^^^^^^^^^^
.. doxygenvariable:: ixion::column_unset

column_upper_bound
^^^^^^^^^^^^^^^^^^
.. doxygenvariable:: ixion::column_upper_bound

empty_string_id
^^^^^^^^^^^^^^^
.. doxygenvariable:: ixion::empty_string_id

global_scope
^^^^^^^^^^^^
.. doxygenvariable:: ixion::global_scope

invalid_sheet
^^^^^^^^^^^^^
.. doxygenvariable:: ixion::invalid_sheet

row_unset
^^^^^^^^^
.. doxygenvariable:: ixion::row_unset

row_upper_bound
^^^^^^^^^^^^^^^
.. doxygenvariable:: ixion::row_upper_bound


Functions
---------

calculate_sorted_cells
^^^^^^^^^^^^^^^^^^^^^^
.. doxygenfunction:: ixion::calculate_sorted_cells(model_context &cxt, const std::vector< abs_range_t > &formula_cells, size_t thread_count)

create_formula_error_tokens
^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. doxygenfunction:: ixion::create_formula_error_tokens(model_context &cxt, std::string_view src_formula, std::string_view error)

get_api_version_major
^^^^^^^^^^^^^^^^^^^^^
.. doxygenfunction:: ixion::get_api_version_major()

get_api_version_minor
^^^^^^^^^^^^^^^^^^^^^
.. doxygenfunction:: ixion::get_api_version_minor()

get_current_time
^^^^^^^^^^^^^^^^
.. doxygenfunction:: ixion::get_current_time()

get_formula_error_name
^^^^^^^^^^^^^^^^^^^^^^
.. doxygenfunction:: ixion::get_formula_error_name(formula_error_t fe)

get_formula_function_name
^^^^^^^^^^^^^^^^^^^^^^^^^
.. doxygenfunction:: ixion::get_formula_function_name(formula_function_t func)

get_formula_function_opcode
^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. doxygenfunction:: ixion::get_formula_function_opcode(std::string_view s)

get_formula_opcode_name
^^^^^^^^^^^^^^^^^^^^^^^
.. doxygenfunction:: ixion::get_formula_opcode_name(fopcode_t oc)

get_formula_opcode_string
^^^^^^^^^^^^^^^^^^^^^^^^^
.. doxygenfunction:: ixion::get_formula_opcode_string(fopcode_t oc)

get_version_major
^^^^^^^^^^^^^^^^^
.. doxygenfunction:: ixion::get_version_major()

get_version_micro
^^^^^^^^^^^^^^^^^
.. doxygenfunction:: ixion::get_version_micro()

get_version_minor
^^^^^^^^^^^^^^^^^
.. doxygenfunction:: ixion::get_version_minor()

intrusive_ptr_add_ref
^^^^^^^^^^^^^^^^^^^^^
.. doxygenfunction:: ixion::intrusive_ptr_add_ref(formula_tokens_store *p)

intrusive_ptr_release
^^^^^^^^^^^^^^^^^^^^^
.. doxygenfunction:: ixion::intrusive_ptr_release(formula_tokens_store *p)

is_valid_sheet
^^^^^^^^^^^^^^
.. doxygenfunction:: ixion::is_valid_sheet(sheet_t sheet)

parse_formula_string
^^^^^^^^^^^^^^^^^^^^
.. doxygenfunction:: ixion::parse_formula_string(model_context &cxt, const abs_address_t &pos, const formula_name_resolver &resolver, std::string_view formula)

print_formula_token
^^^^^^^^^^^^^^^^^^^
.. doxygenfunction:: ixion::print_formula_token(const model_context &cxt, const abs_address_t &pos, const formula_name_resolver &resolver, const formula_token &token)

print_formula_token
^^^^^^^^^^^^^^^^^^^
.. doxygenfunction:: ixion::print_formula_token(const print_config &config, const model_context &cxt, const abs_address_t &pos, const formula_name_resolver &resolver, const formula_token &token)

print_formula_tokens
^^^^^^^^^^^^^^^^^^^^
.. doxygenfunction:: ixion::print_formula_tokens(const model_context &cxt, const abs_address_t &pos, const formula_name_resolver &resolver, const formula_tokens_t &tokens)

print_formula_tokens
^^^^^^^^^^^^^^^^^^^^
.. doxygenfunction:: ixion::print_formula_tokens(const print_config &config, const model_context &cxt, const abs_address_t &pos, const formula_name_resolver &resolver, const formula_tokens_t &tokens)

query_and_sort_dirty_cells
^^^^^^^^^^^^^^^^^^^^^^^^^^
.. doxygenfunction:: ixion::query_and_sort_dirty_cells(model_context &cxt, const abs_range_set_t &modified_cells, const abs_range_set_t *dirty_formula_cells=nullptr)

query_dirty_cells
^^^^^^^^^^^^^^^^^
.. doxygenfunction:: ixion::query_dirty_cells(model_context &cxt, const abs_address_set_t &modified_cells)

register_formula_cell
^^^^^^^^^^^^^^^^^^^^^
.. doxygenfunction:: ixion::register_formula_cell(model_context &cxt, const abs_address_t &pos, const formula_cell *cell=nullptr)

to_bool
^^^^^^^
.. doxygenfunction:: ixion::to_bool(std::string_view s)

to_double
^^^^^^^^^
.. doxygenfunction:: ixion::to_double(std::string_view s)

to_formula_error_type
^^^^^^^^^^^^^^^^^^^^^
.. doxygenfunction:: ixion::to_formula_error_type(std::string_view s)

unregister_formula_cell
^^^^^^^^^^^^^^^^^^^^^^^
.. doxygenfunction:: ixion::unregister_formula_cell(model_context &cxt, const abs_address_t &pos)


Struct
------

abs_address_t
^^^^^^^^^^^^^
.. doxygenstruct:: ixion::abs_address_t
   :members:

abs_range_t
^^^^^^^^^^^
.. doxygenstruct:: ixion::abs_range_t
   :members:

abs_rc_address_t
^^^^^^^^^^^^^^^^
.. doxygenstruct:: ixion::abs_rc_address_t
   :members:

abs_rc_range_t
^^^^^^^^^^^^^^
.. doxygenstruct:: ixion::abs_rc_range_t
   :members:

address_t
^^^^^^^^^
.. doxygenstruct:: ixion::address_t
   :members:

column_block_shape_t
^^^^^^^^^^^^^^^^^^^^
.. doxygenstruct:: ixion::column_block_shape_t
   :members:

config
^^^^^^
.. doxygenstruct:: ixion::config
   :members:

formula_group_t
^^^^^^^^^^^^^^^
.. doxygenstruct:: ixion::formula_group_t
   :members:

formula_name_t
^^^^^^^^^^^^^^
.. doxygenstruct:: ixion::formula_name_t
   :members:

formula_token
^^^^^^^^^^^^^
.. doxygenstruct:: ixion::formula_token
   :members:

named_expression_t
^^^^^^^^^^^^^^^^^^
.. doxygenstruct:: ixion::named_expression_t
   :members:

print_config
^^^^^^^^^^^^
.. doxygenstruct:: ixion::print_config
   :members:

range_t
^^^^^^^
.. doxygenstruct:: ixion::range_t
   :members:

rc_address_t
^^^^^^^^^^^^
.. doxygenstruct:: ixion::rc_address_t
   :members:

rc_range_t
^^^^^^^^^^
.. doxygenstruct:: ixion::rc_range_t
   :members:

rc_size_t
^^^^^^^^^
.. doxygenstruct:: ixion::rc_size_t
   :members:

table_t
^^^^^^^
.. doxygenstruct:: ixion::table_t
   :members:


Classes
-------

abs_address_iterator
^^^^^^^^^^^^^^^^^^^^
.. doxygenclass:: ixion::abs_address_iterator
   :members:

cell_access
^^^^^^^^^^^
.. doxygenclass:: ixion::cell_access
   :members:

dirty_cell_tracker
^^^^^^^^^^^^^^^^^^
.. doxygenclass:: ixion::dirty_cell_tracker
   :members:

document
^^^^^^^^
.. doxygenclass:: ixion::document
   :members:

file_not_found
^^^^^^^^^^^^^^
.. doxygenclass:: ixion::file_not_found
   :members:

formula_cell
^^^^^^^^^^^^
.. doxygenclass:: ixion::formula_cell
   :members:

formula_error
^^^^^^^^^^^^^
.. doxygenclass:: ixion::formula_error
   :members:

formula_name_resolver
^^^^^^^^^^^^^^^^^^^^^
.. doxygenclass:: ixion::formula_name_resolver
   :members:

formula_registration_error
^^^^^^^^^^^^^^^^^^^^^^^^^^
.. doxygenclass:: ixion::formula_registration_error
   :members:

formula_result
^^^^^^^^^^^^^^
.. doxygenclass:: ixion::formula_result
   :members:

formula_tokens_store
^^^^^^^^^^^^^^^^^^^^
.. doxygenclass:: ixion::formula_tokens_store
   :members:

general_error
^^^^^^^^^^^^^
.. doxygenclass:: ixion::general_error
   :members:

matrix
^^^^^^
.. doxygenclass:: ixion::matrix
   :members:

model_context
^^^^^^^^^^^^^
.. doxygenclass:: ixion::model_context
   :members:

model_context_error
^^^^^^^^^^^^^^^^^^^
.. doxygenclass:: ixion::model_context_error
   :members:

model_iterator
^^^^^^^^^^^^^^
.. doxygenclass:: ixion::model_iterator
   :members:

named_expressions_iterator
^^^^^^^^^^^^^^^^^^^^^^^^^^
.. doxygenclass:: ixion::named_expressions_iterator
   :members:

not_implemented_error
^^^^^^^^^^^^^^^^^^^^^
.. doxygenclass:: ixion::not_implemented_error
   :members:

numeric_matrix
^^^^^^^^^^^^^^
.. doxygenclass:: ixion::numeric_matrix
   :members:

values_t
^^^^^^^^
.. doxygenclass:: ixion::values_t
   :members:


Child namespaces
----------------
.. toctree::
   :maxdepth: 1

   draft/index.rst
   iface/index.rst