
AM_CPPFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/include \
	-DIXION_BUILD \
	-DSRCDIR=\""$(top_srcdir)"\" \
	$(MDDS_CFLAGS) \
	$(BOOST_CPPFLAGS)

if WINDOWS

AM_CPPFLAGS += -D_USE_MATH_DEFINES

endif # WINDOWS

if BUILD_VULKAN

AM_CPPFLAGS += -DBUILD_VULKAN

endif # BUILD_VULKAN

check_PROGRAMS = \
	column-store-test \
	document-test \
	general-test \
	model-context-test \
	sheet-sort-test \
	ixion-test-track-deps \
	compute-engine-test \
	dirty-cell-tracker-test \
	name-resolver-test \
	version-test

lib_LTLIBRARIES = libixion-@IXION_API_VERSION@.la
libixion_@IXION_API_VERSION@_la_SOURCES = \
	address.cpp \
	address_range.cpp \
	calc_status.hpp \
	cell.cpp \
	cell_access.cpp \
	column_store_type.hpp \
	compute_engine.cpp \
	config.cpp \
	debug.hpp \
	debug.cpp \
	dirty_cell_tracker.cpp \
	document.cpp \
	exceptions.cpp \
	formula.cpp \
	formula_calc.cpp \
	formula_function_opcode.cpp \
	formula_functions.hpp \
	formula_functions.cpp \
	formula_groups.hpp \
	formula_groups.cpp \
	formula_interpreter.hpp \
	formula_interpreter.cpp \
	formula_lexer.hpp \
	formula_lexer.cpp \
	formula_name_resolver.cpp \
	formula_opcode.cpp \
	formula_parser.hpp \
	formula_parser.cpp \
	formula_result.cpp \
	formula_tokens.cpp \
	formula_value_stack.hpp \
	formula_value_stack.cpp \
	global.cpp \
	grid_dumper.hpp \
	grid_dumper.cpp \
	impl_types.hpp \
	impl_types.cpp \
	info.cpp \
	lexer_tokens.hpp \
	lexer_tokens.cpp \
	matrix.cpp \
	model_context.cpp \
	model_context_impl.hpp \
	model_context_impl.cpp \
	string_id_pool.hpp \
	string_id_pool.cpp \
	inline_string_pool.hpp \
	inline_string_pool.cpp \
	model_cell_range.cpp \
	model_iterator.cpp \
	model_types.hpp \
	model_types.cpp \
	module.cpp \
	named_expressions_iterator.cpp \
	queue_entry.hpp \
	queue_entry.cpp \
	table.cpp \
	table_store.hpp \
	table_store.cpp \
	types.cpp \
	utf8.hpp \
	utf8.cpp \
	utils.hpp \
	utils.cpp \
	sheet_sort.hpp \
	sheet_sort.cpp \
	sheet_store.hpp \
	sheet_store.cpp \
	interface.cpp

if BUILD_VULKAN

# VULKAN module

pkglib_LTLIBRARIES = ixion-@IXION_API_VERSION@-vulkan.la
ixion_@IXION_API_VERSION@_vulkan_la_SOURCES = \
	compute_engine_vulkan.hpp \
	compute_engine_vulkan.cpp \
	vulkan_obj.hpp \
	vulkan_obj.cpp

ixion_@IXION_API_VERSION@_vulkan_la_LDFLAGS = \
	-module -avoid-version -export-symbols-regex \
	register_module

ixion_@IXION_API_VERSION@_vulkan_la_LIBADD = \
	libixion-@IXION_API_VERSION@.la \
	$(VULKAN_LIBS)

ixion_@IXION_API_VERSION@_vulkan_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	$(VULKAN_CPPFLAGS)

endif # BUILD_VULKAN

if IXION_THREADS

libixion_@IXION_API_VERSION@_la_SOURCES += \
	cell_queue_manager.hpp \
	cell_queue_manager.cpp

endif

libixion_@IXION_API_VERSION@_la_LDFLAGS = -no-undefined

if !WINDOWS

libixion_@IXION_API_VERSION@_la_LDFLAGS += -Wl,-rpath,'$$ORIGIN' -Wl,-rpath,'$$ORIGIN/$(PACKAGE)'

endif # !WINDOWS

column_store_test_SOURCES = column_store_test.cpp
column_store_test_LDADD = \
	libixion-@IXION_API_VERSION@.la \
	../test/libixion-test.a

document_test_SOURCES = document_test.cpp
document_test_LDADD = \
	libixion-@IXION_API_VERSION@.la \
	../test/libixion-test.a

general_test_SOURCES = general_test.cpp
general_test_LDADD = \
	libixion-@IXION_API_VERSION@.la \
	../test/libixion-test.a \
	$(BOOST_PROGRAM_OPTIONS_LIBS)

model_context_test_SOURCES = model_context_test.cpp
model_context_test_LDADD = \
	libixion-@IXION_API_VERSION@.la \
	../test/libixion-test.a

# The tested classes are internal to the library and their symbols stay
# hidden, so their sources get compiled into the test binary directly.
sheet_sort_test_SOURCES = \
	sheet_sort_test.cpp \
	sheet_sort.cpp \
	sheet_store.cpp \
	formula_groups.cpp
# Give the target its own flags so that the sources shared with the library
# compile into their own objects.
sheet_sort_test_CPPFLAGS = $(AM_CPPFLAGS)
sheet_sort_test_LDADD = \
	libixion-@IXION_API_VERSION@.la \
	../test/libixion-test.a

name_resolver_test_SOURCES = name_resolver_test.cpp
name_resolver_test_LDADD = \
	libixion-@IXION_API_VERSION@.la \
	../test/libixion-test.a \
	$(BOOST_PROGRAM_OPTIONS_LIBS)

ixion_test_track_deps_SOURCES = ixion_test_track_deps.cpp
ixion_test_track_deps_LDADD = \
	libixion-@IXION_API_VERSION@.la \
	../test/libixion-test.a \
	$(BOOST_PROGRAM_OPTIONS_LIBS)

compute_engine_test_SOURCES = compute_engine_test.cpp
compute_engine_test_LDADD = \
	../test/libixion-test.a \
	libixion-@IXION_API_VERSION@.la

dirty_cell_tracker_test_SOURCES = dirty_cell_tracker_test.cpp
dirty_cell_tracker_test_LDADD = \
	../test/libixion-test.a \
	libixion-@IXION_API_VERSION@.la

version_test_SOURCES = version_test.cpp
version_test_LDADD = \
	../test/libixion-test.a \
	libixion-@IXION_API_VERSION@.la

AM_TESTS_ENVIRONMENT =

TESTS = \
	column-store-test \
	document-test \
	general-test \
	model-context-test \
	sheet-sort-test \
	ixion-test-track-deps \
	compute-engine-test \
	dirty-cell-tracker-test \
	name-resolver-test \
	version-test
