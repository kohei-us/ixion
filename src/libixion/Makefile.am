AM_CPPFLAGS = -I$(top_srcdir)/include -DIXION_BUILD $(MDDS_CFLAGS)

check_PROGRAMS = ixion-test

lib_LTLIBRARIES = libixion-@IXION_API_VERSION@.la
libixion_@IXION_API_VERSION@_la_SOURCES = \
	address.cpp \
	cell.cpp \
	cell_queue_manager.cpp \
	config.cpp \
	depends_tracker.hpp \
	depends_tracker.cpp \
	exceptions.cpp \
	formula.cpp \
	formula_function_opcode.cpp \
	formula_functions.hpp \
	formula_functions.cpp \
	formula_interpreter.hpp \
	formula_interpreter.cpp \
	formula_lexer.hpp \
	formula_lexer.cpp \
	formula_name_resolver.cpp \
	formula_parser.cpp \
	formula_result.cpp \
	formula_tokens.cpp \
	formula_value_stack.hpp \
	formula_value_stack.cpp \
	function_objects.hpp \
	function_objects.cpp \
	global.cpp \
	info.cpp \
	lexer_tokens.cpp \
	matrix.cpp \
	mem_str_buf.cpp \
	model_context.cpp \
	cell_listener_tracker.cpp \
	table.cpp \
	types.cpp \
	workbook.hpp \
	workbook.cpp \
	interface.cpp


libixion_@IXION_API_VERSION@_la_LDFLAGS = -no-undefined
libixion_@IXION_API_VERSION@_la_LIBADD = $(BOOST_THREAD_LIBS)

EXTRA_DIST = makefile.mk

ixion_test_SOURCES = ixion_test.cpp
ixion_test_LDADD = libixion-@IXION_API_VERSION@.la \
					 $(BOOST_THREAD_LIBS) $(BOOST_PROGRAM_OPTIONS_LIBS)

TESTS = ixion-test
