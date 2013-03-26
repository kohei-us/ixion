INCLUDES = -I$(top_srcdir)/include

lib_LTLIBRARIES = libixion-@IXION_API_VERSION@.la
libixion_@IXION_API_VERSION@_la_SOURCES = \
	address.cpp \
	cell.cpp \
	cell_queue_manager.cpp \
	config.cpp \
	depends_tracker.cpp \
	exceptions.cpp \
	formula.cpp \
	formula_function_opcode.cpp \
	formula_functions.cpp \
	formula_interpreter.cpp \
	formula_lexer.cpp \
	formula_name_resolver.cpp \
	formula_parser.cpp \
	formula_result.cpp \
	formula_tokens.cpp \
	function_objects.cpp \
	global.cpp \
	lexer_tokens.cpp \
	matrix.cpp \
	mem_str_buf.cpp \
	model_context.cpp \
	multi_type_vector_trait.hpp \
	cell_listener_tracker.cpp \
	session_handler.cpp \
	types.cpp \
	workbook.hpp \
	workbook.cpp

libixion_@IXION_API_VERSION@_la_LDFLAGS = -no-undefined

libixion_@IXION_API_VERSION@_la_LIBADD = $(BOOST_THREAD_LIBS)

EXTRA_DIST = makefile.mk
