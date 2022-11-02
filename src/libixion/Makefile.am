
AM_CPPFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/include \
	-DIXION_BUILD \
	$(MDDS_CFLAGS) \
	$(BOOST_CPPFLAGS)

if BUILD_VULKAN

AM_CPPFLAGS += -DBUILD_VULKAN

endif # BUILD_VULKAN

check_PROGRAMS = \
	document-test \
	ixion-test \
	ixion-test-track-deps \
	compute-engine-test \
	dirty-cell-tracker-test

lib_LTLIBRARIES = libixion-@IXION_API_VERSION@.la
libixion_@IXION_API_VERSION@_la_SOURCES = \
	address.cpp \
	address_iterator.cpp \
	calc_status.hpp \
	calc_status.cpp \
	cell.cpp \
	cell_access.cpp \
	column_store_type.hpp \
	compute_engine.cpp \
	concrete_formula_tokens.hpp \
	concrete_formula_tokens.cpp \
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
	formula_interpreter.hpp \
	formula_interpreter.cpp \
	formula_lexer.hpp \
	formula_lexer.cpp \
	formula_name_resolver.cpp \
	formula_parser.hpp \
	formula_parser.cpp \
	formula_result.cpp \
	formula_tokens.cpp \
	formula_value_stack.hpp \
	formula_value_stack.cpp \
	global.cpp \
	info.cpp \
	lexer_tokens.hpp \
	lexer_tokens.cpp \
	matrix.cpp \
	mem_str_buf.cpp \
	model_context.cpp \
	model_context_impl.hpp \
	model_context_impl.cpp \
	model_iterator.cpp \
	model_types.hpp \
	model_types.cpp \
	module.cpp \
	named_expressions_iterator.cpp \
	queue_entry.hpp \
	queue_entry.cpp \
	table.cpp \
	types.cpp \
	utf8.hpp \
	utf8.cpp \
	utils.hpp \
	utils.cpp \
	workbook.hpp \
	workbook.cpp \
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

libixion_@IXION_API_VERSION@_la_LDFLAGS = \
	-no-undefined \
	-Wl,-rpath,'$$ORIGIN' -Wl,-rpath,'$$ORIGIN/$(PACKAGE)' \
	$(BOOST_FILESYSTEM_LDFLAGS)

libixion_@IXION_API_VERSION@_la_LIBADD = \
	$(BOOST_FILESYSTEM_LIBS)

EXTRA_DIST = makefile.mk

document_test_SOURCES = document_test.cpp
document_test_LDADD = \
	libixion-@IXION_API_VERSION@.la \
	../test/libixion-test.a

ixion_test_SOURCES = ixion_test.cpp
ixion_test_LDADD = \
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

AM_TESTS_ENVIRONMENT =

TESTS = \
	document-test \
	ixion-test \
	ixion-test-track-deps \
	compute-engine-test \
	dirty-cell-tracker-test
