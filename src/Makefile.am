SUBDIRS = libixion .

INCLUDES = -I$(top_srcdir)/include

bin_PROGRAMS = ixion-parser ixion-sorter ixion-test
ixion_parser_SOURCES = ixion_parser.cpp
ixion_parser_LDADD = libixion/libixion-@IXION_API_VERSION@.la \
					 $(BOOST_THREAD_LIBS) $(BOOST_PROGRAM_OPTIONS_LIBS)
ixion_parser_CPPFLAGS = -I$(top_builddir)/lib/libixion/libixion.la

ixion_sorter_SOURCES = ixion_sorter.cpp
ixion_sorter_LDADD = libixion/libixion-@IXION_API_VERSION@.la \
					 $(BOOST_THREAD_LIBS) $(BOOST_PROGRAM_OPTIONS_LIBS)
ixion_sorter_CPPFLAGS = -I$(top_builddir)/lib/libixion/libixion.la

ixion_test_SOURCES = ixion_test.cpp
ixion_test_LDADD = libixion/libixion-@IXION_API_VERSION@.la
ixion_test_CPPFLAGS = -I$(top_builddir)/lib/libixion/libixion.la

TESTS = ixion-test $(top_srcdir)/test/ixion-parser-test.sh
