
Formula Engine
==============

.. doxygenfunction:: ixion::parse_formula_string
.. doxygenfunction:: ixion::print_formula_tokens(const model_context &cxt, const abs_address_t &pos, const formula_name_resolver &resolver, const formula_tokens_t &tokens)
.. doxygenfunction:: ixion::print_formula_tokens(const print_config &config, const model_context &cxt, const abs_address_t &pos, const formula_name_resolver &resolver, const formula_tokens_t &tokens)
.. doxygenfunction:: ixion::register_formula_cell
.. doxygenfunction:: ixion::unregister_formula_cell
.. doxygenfunction:: ixion::query_dirty_cells
.. doxygenfunction:: ixion::query_and_sort_dirty_cells
.. doxygenfunction:: ixion::calculate_sorted_cells


Formula Functions
=================

.. doxygenenum:: ixion::formula_function_t
.. doxygenfunction:: ixion::get_formula_function_name
.. doxygenfunction:: ixion::get_formula_function_opcode
