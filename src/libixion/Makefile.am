INCLUDES = -I$(top_srcdir)/include

lib_LTLIBRARIES = libixion-@IXION_API_VERSION@.la
libixion_@IXION_API_VERSION@_la_SOURCES = \
	address.cpp \
	cell.cpp \
	cell_queue_manager.cpp \
	depends_tracker.cpp \
	formula_functions.cpp \
	formula_interpreter.cpp \
	formula_lexer.cpp \
	formula_name_resolver.cpp \
	formula_parser.cpp \
	formula_result.cpp \
	formula_tokens.cpp \
	global.cpp \
	lexer_tokens.cpp \
	matrix.cpp \
	mem_str_buf.cpp \
	model_context.cpp \
	model_parser.cpp \
	sort_input_parser.cpp

libixion_@IXION_API_VERSION@_la_LDFLAGS = -no-undefined

libixion_@IXION_API_VERSION@_la_LIBADD = $(BOOST_THREAD_LIBS)

EXTRA_DIST = makefile.mk
