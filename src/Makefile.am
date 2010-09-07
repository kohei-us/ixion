SUBDIRS = libixion .

INCLUDES = -I$(top_srcdir)/inc

bin_PROGRAMS = ixion-parser ixion-sorter
ixion_parser_SOURCES = ixion_parser.cpp
ixion_parser_LDADD = libixion/libixion-@IXION_API_VERSION@.la \
					 $(BOOST_THREAD_LIBS) $(BOOST_PROGRAM_OPTIONS_LIBS)
ixion_parser_CPPFLAGS = -I$(top_builddir)/lib/libixion/libixion.la

ixion_sorter_SOURCES = ixion_sorter.cpp
ixion_sorter_LDADD = libixion/libixion-@IXION_API_VERSION@.la \
					 $(BOOST_THREAD_LIBS) $(BOOST_PROGRAM_OPTIONS_LIBS)
ixion_sorter_CPPFLAGS = -I$(top_builddir)/lib/libixion/libixion.la

TESTS_ENVIRONMENT = $(SHELL) ./ixion-parser
TESTS = $(top_srcdir)/test/01-simple-arithmetic.txt \
		$(top_srcdir)/test/02-circular.txt \
		$(top_srcdir)/test/03-expression.txt \
		$(top_srcdir)/test/04-function-single.txt \
		$(top_srcdir)/test/05-function-parallel.txt \
		$(top_srcdir)/test/06-function-wait-simple.txt \
		$(top_srcdir)/test/07-fraction-numbers.txt
