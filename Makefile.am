SUBDIRS = src include doc_example
ACLOCAL_AMFLAGS = -I m4

pcfiles = libixion-@IXION_API_VERSION@.pc

pkgconfig_DATA = $(pcfiles)
pkgconfigdir = $(libdir)/pkgconfig

bin_data = \
	bin/gen-function-list.py \
	bin/gen-gfm-release-table.sh \
	bin/gen-spirv-shader-arrays.py \
	bin/run-python-test-osx.sh \
	bin/run-python.sh \
	misc/function-specs.yaml

test_data = \
	test/00-command-exit.txt \
	test/01-concat-operator.txt \
	test/01-exponent-operator.txt \
	test/01-inline-array-div.txt \
	test/01-inline-array-exp.txt \
	test/01-inline-array-mul.txt \
	test/01-inline-array-op-lhs.txt \
	test/01-inline-array-op-rhs.txt \
	test/01-error-tokens.txt \
	test/01-inline-array-string-concat.txt \
	test/01-inline-array.txt \
	test/01-ranged-definitions-dups.txt \
	test/01-simple-arithmetic.txt \
	test/02-circular-01.txt \
	test/02-circular-02.txt \
	test/02-circular-03.txt \
	test/03-expression.txt \
	test/03-leading-signs.txt \
	test/04-function-abs.txt \
	test/04-function-and-boolean.txt \
	test/04-function-and.txt \
	test/04-function-average.txt \
	test/04-function-column-row.txt \
	test/04-function-columns-rows.txt \
	test/04-function-concatenate.txt \
	test/04-function-count-formula.txt \
	test/04-function-iserr.txt \
	test/04-function-count.txt \
	test/04-function-counta-edit.txt \
	test/04-function-counta-static-args.txt \
	test/04-function-countblank.txt \
	test/04-function-exact.txt \
	test/04-function-find.txt \
	test/04-function-invalid-name.txt \
	test/04-function-isblank.txt \
	test/04-function-iserror.txt \
	test/04-function-iseven.txt \
	test/04-function-isformula.txt \
	test/04-function-islogical.txt \
	test/04-function-isna.txt \
	test/04-function-isnumber.txt \
	test/04-function-isref.txt \
	test/04-function-istext.txt \
	test/04-function-left-utf8.txt \
	test/04-function-left.txt \
	test/04-function-len.txt \
	test/04-function-logical.txt \
	test/04-function-median.txt \
	test/04-function-mid-utf8.txt \
	test/04-function-mid.txt \
	test/04-function-mmult-inline.txt \
	test/04-function-mmult.txt \
	test/04-function-mode.txt \
	test/04-function-n.txt \
	test/04-function-nested.txt \
	test/04-function-not-implemented.txt \
	test/04-function-or.txt \
	test/04-function-pi-int.txt \
	test/04-function-replace.txt \
	test/04-function-rept.txt \
	test/04-function-right-utf8.txt \
	test/04-function-right.txt \
	test/04-function-sheet.txt \
	test/04-function-sheets.txt \
	test/04-function-single.txt \
	test/04-function-substitute.txt \
	test/04-function-t.txt \
	test/04-function-textjoin.txt \
	test/04-function-trim.txt \
	test/04-function-true-false.txt \
	test/04-function-type.txt \
	test/05-range-reference.txt \
	test/06-range-reference-basic-01.txt \
	test/06-range-reference-basic-02.txt \
	test/06-range-reference-circular-01.txt \
	test/06-range-reference-unordered.txt \
	test/06-range-reference-whole-column.txt \
	test/06-range-reference-whole-row.txt \
	test/07-fraction-numbers.txt \
	test/08-boolean-cells.txt \
	test/08-numeric-cells.txt \
	test/09-string-cells.txt \
	test/10-shared-formulas-01.txt \
	test/11-reference-to-numeric-cell-01.txt \
	test/12-inline-string-01.txt \
	test/13-relational-operators-01.txt \
	test/13-relational-operators-02.txt \
	test/13-relational-operators-03.txt \
	test/20-table-reference-01.txt \
	test/20-table-reference-02.txt \
	test/20-table-reference-03.txt \
	test/20-table-reference-04.txt \
	test/21-named-exp-workbook-01.txt \
	test/21-named-exp-worksheet-01.txt \
	test/22-formulas-with-cached-results.txt \
	test/22-grouped-formulas-with-cached-results.txt \
	test/23-dump-sheet.txt \
	test/24-copy-sheet-array-formula.txt \
	test/25-copy-sheet-multi-column-group.txt \
	test/26-copy-sheet-multi-column-group-on-copy.txt \
	test/expected/model-context/dump-sheet-simple.txt \
	test/expected/model-context/dump-sheet-verbose.txt \
	test/expected/model-context/dump-sheet-verbose-r1c1.txt \
	test/expected/sheet-view/dump-simple.txt \
	test/expected/sheet-view/dump-verbose.txt \
	test/parser-test-func.sh \
	test/parser-test-t0.sh \
	test/parser-test-t1.sh \
	test/parser-test-t2.sh \
	test/parser-test-t3.sh \
	test/parser-test-t4.sh \
	test/parser-test-t5.sh \
	test/parser-test-t6.sh \
	test/parser-test-t7.sh \
	test/parser-test-t8.sh \
	test/python/document.py \
	test/python/module.py \
	test/thread/function-parallel.txt \
	test/thread/function-wait-simple.txt

doc_data = \
	doc/_static/custom.css \
	doc/conf.py \
	doc/cpp/index.rst \
	doc/cpp/ixion/class-abs_address_range.rst \
	doc/cpp/ixion/class-cell_access.rst \
	doc/cpp/ixion/class-dirty_cell_tracker.rst \
	doc/cpp/ixion/class-document.rst \
	doc/cpp/ixion/class-file_not_found.rst \
	doc/cpp/ixion/class-formula_cell.rst \
	doc/cpp/ixion/class-formula_error.rst \
	doc/cpp/ixion/class-formula_name_resolver.rst \
	doc/cpp/ixion/class-formula_result.rst \
	doc/cpp/ixion/class-formula_tokens_store.rst \
	doc/cpp/ixion/class-general_error.rst \
	doc/cpp/ixion/class-matrix.rst \
	doc/cpp/ixion/class-model_cell_range.rst \
	doc/cpp/ixion/class-model_context.rst \
	doc/cpp/ixion/class-model_context_error.rst \
	doc/cpp/ixion/class-model_context_loader.rst \
	doc/cpp/ixion/class-model_iterator.rst \
	doc/cpp/ixion/class-named_expressions_iterator.rst \
	doc/cpp/ixion/class-named_expressions_range.rst \
	doc/cpp/ixion/class-not_implemented_error.rst \
	doc/cpp/ixion/class-numeric_matrix.rst \
	doc/cpp/ixion/class-sheet_view.rst \
	doc/cpp/ixion/class-values_t.rst \
	doc/cpp/ixion/enum-cell_t.rst \
	doc/cpp/ixion/enum-cell_value_t.rst \
	doc/cpp/ixion/enum-column_block_t.rst \
	doc/cpp/ixion/enum-display_sheet_t.rst \
	doc/cpp/ixion/enum-fopcode_t.rst \
	doc/cpp/ixion/enum-formula_error_t.rst \
	doc/cpp/ixion/enum-formula_event_t.rst \
	doc/cpp/ixion/enum-formula_function_t.rst \
	doc/cpp/ixion/enum-formula_name_resolver_t.rst \
	doc/cpp/ixion/enum-formula_result_wait_policy_t.rst \
	doc/cpp/ixion/enum-rc_direction_t.rst \
	doc/cpp/ixion/enum-sheet_dump_mode_t.rst \
	doc/cpp/ixion/enum-sort_order_t.rst \
	doc/cpp/ixion/enum-table_area_t.rst \
	doc/cpp/ixion/enum-value_t.rst \
	doc/cpp/ixion/function-calculate_sorted_cells.rst \
	doc/cpp/ixion/function-create_formula_error_tokens.rst \
	doc/cpp/ixion/function-get_api_version_major.rst \
	doc/cpp/ixion/function-get_api_version_minor.rst \
	doc/cpp/ixion/function-get_current_time.rst \
	doc/cpp/ixion/function-get_formula_error_name.rst \
	doc/cpp/ixion/function-get_formula_function_name.rst \
	doc/cpp/ixion/function-get_formula_function_opcode.rst \
	doc/cpp/ixion/function-get_formula_opcode_name.rst \
	doc/cpp/ixion/function-get_formula_opcode_string.rst \
	doc/cpp/ixion/function-get_version_major.rst \
	doc/cpp/ixion/function-get_version_micro.rst \
	doc/cpp/ixion/function-get_version_minor.rst \
	doc/cpp/ixion/function-intrusive_ptr_add_ref.rst \
	doc/cpp/ixion/function-intrusive_ptr_release.rst \
	doc/cpp/ixion/function-is_valid_sheet.rst \
	doc/cpp/ixion/function-parse_formula_string.rst \
	doc/cpp/ixion/function-print_formula_token-2.rst \
	doc/cpp/ixion/function-print_formula_token.rst \
	doc/cpp/ixion/function-print_formula_tokens-2.rst \
	doc/cpp/ixion/function-print_formula_tokens.rst \
	doc/cpp/ixion/function-query_and_sort_dirty_cells.rst \
	doc/cpp/ixion/function-query_dirty_cells.rst \
	doc/cpp/ixion/function-to_bool.rst \
	doc/cpp/ixion/function-to_double.rst \
	doc/cpp/ixion/function-to_formula_error_type.rst \
	doc/cpp/ixion/iface/class-session_handler.rst \
	doc/cpp/ixion/iface/index.rst \
	doc/cpp/ixion/index.rst \
	doc/cpp/ixion/struct-abs_address_t.rst \
	doc/cpp/ixion/struct-abs_range_t.rst \
	doc/cpp/ixion/struct-abs_rc_address_t.rst \
	doc/cpp/ixion/struct-abs_rc_range_t.rst \
	doc/cpp/ixion/struct-address_t.rst \
	doc/cpp/ixion/struct-column_block_shape_t.rst \
	doc/cpp/ixion/struct-config.rst \
	doc/cpp/ixion/struct-formula_group_t.rst \
	doc/cpp/ixion/struct-formula_name_t.rst \
	doc/cpp/ixion/struct-formula_token.rst \
	doc/cpp/ixion/struct-named_expression_t.rst \
	doc/cpp/ixion/struct-print_config.rst \
	doc/cpp/ixion/struct-range_t.rst \
	doc/cpp/ixion/struct-rc_address_t.rst \
	doc/cpp/ixion/struct-rc_range_t.rst \
	doc/cpp/ixion/struct-rc_size_t.rst \
	doc/cpp/ixion/struct-sort_key_t.rst \
	doc/cpp/ixion/struct-string_id_t.rst \
	doc/cpp/ixion/struct-table_ref_t.rst \
	doc/cpp/ixion/struct-table_t.rst \
	doc/cpp/ixion/typedef-abs_address_iterator.rst \
	doc/cpp/ixion/typedef-abs_address_set_t.rst \
	doc/cpp/ixion/typedef-abs_range_set_t.rst \
	doc/cpp/ixion/typedef-abs_rc_range_set_t.rst \
	doc/cpp/ixion/typedef-calc_status_ptr_t.rst \
	doc/cpp/ixion/typedef-col_t.rst \
	doc/cpp/ixion/typedef-column_block_callback_t.rst \
	doc/cpp/ixion/typedef-column_block_handle.rst \
	doc/cpp/ixion/typedef-formula_tokens_store_ptr_t.rst \
	doc/cpp/ixion/typedef-formula_tokens_t.rst \
	doc/cpp/ixion/typedef-rc_t.rst \
	doc/cpp/ixion/typedef-row_t.rst \
	doc/cpp/ixion/typedef-sheet_t.rst \
	doc/cpp/ixion/typedef-sort_keys_t.rst \
	doc/cpp/ixion/typedef-table_areas_t.rst \
	doc/cpp/ixion/variable-column_unset.rst \
	doc/cpp/ixion/variable-column_upper_bound.rst \
	doc/cpp/ixion/variable-empty_string_id.rst \
	doc/cpp/ixion/variable-global_scope.rst \
	doc/cpp/ixion/variable-invalid_sheet.rst \
	doc/cpp/ixion/variable-row_unset.rst \
	doc/cpp/ixion/variable-row_upper_bound.rst \
	doc/doxygen.conf \
	doc/environment.yml \
	doc/functions.rst \
	doc/images/concepts-api-levels.fodg \
	doc/images/concepts-api-levels.svg \
	doc/images/concepts-calculation.fodg \
	doc/images/concepts-calculation.svg \
	doc/images/concepts-cells-and-sheets.fodg \
	doc/images/concepts-cells-and-sheets.svg \
	doc/images/concepts-dependency-tracking.fodg \
	doc/images/concepts-dependency-tracking.svg \
	doc/images/concepts-formula-tokens.fodg \
	doc/images/concepts-formula-tokens.svg \
	doc/images/formula-groups-storage.fodg \
	doc/images/formula-groups-storage.svg \
	doc/images/formula-syntax-reference-parts.fodg \
	doc/images/formula-syntax-reference-parts.svg \
	doc/images/formula-syntax-round-trip.fodg \
	doc/images/formula-syntax-round-trip.svg \
	doc/images/formula-syntax-sheet-relative.fodg \
	doc/images/formula-syntax-sheet-relative.svg \
	doc/images/names-and-tables-expansion.fodg \
	doc/images/names-and-tables-expansion.svg \
	doc/images/names-and-tables-table-anatomy.fodg \
	doc/images/names-and-tables-table-anatomy.svg \
	doc/images/sheet-copy-references.fodg \
	doc/images/sheet-copy-references.svg \
	doc/images/sheet-copy-storage.fodg \
	doc/images/sheet-copy-storage.svg \
	doc/index.rst \
	doc/overview/cell-access.rst \
	doc/overview/concepts.rst \
	doc/overview/document.rst \
	doc/overview/errors-and-volatile.rst \
	doc/overview/formula-groups.rst \
	doc/overview/formula-syntax.rst \
	doc/overview/index.rst \
	doc/overview/model-context-loader.rst \
	doc/overview/model-context.rst \
	doc/overview/names-and-tables.rst \
	doc/overview/sheet-copy.rst \
	doc/overview/sheet-view.rst \
	doc/python/document.rst \
	doc/python/index.rst \
	doc/python/quickstart.rst \
	doc/python/sheet.rst \
	doc/requirements.txt

EXTRA_DIST = \
	CHANGELOG \
	LICENSE \
	libixion.pc.in \
	autogen.sh \
	$(bin_data) \
	$(test_data) \
	$(doc_data)

.PHONY: distclean-local doc-doxygen doc-sphinx doc doc-open doc-clean

distclean-local:
	rm -rf *.pc

doc-doxygen:
	@echo "Building documentation by doxygen..."
	@rm -rf ./doc/_doxygen
	@cd doc && doxygen doxygen.conf

doc-sphinx:
	@echo "Building documentation by sphinx..."
	@sphinx-build -b html ./doc/ ./doc/_build

doc: doc-doxygen doc-sphinx

doc-open:
	@xdg-open ./doc/_build/index.html

doc-clean:
	@rm -rf ./doc/_build ./doc/_doxygen
