SUBDIRS = libixion

AM_CPPFLAGS = -I$(top_srcdir)/include $(MDDS_CFLAGS)

bin_PROGRAMS = ixion-parser ixion-sorter

ixion_parser_SOURCES = \
	ixion_parser.cpp \
	model_parser.hpp \
	model_parser.cpp

ixion_parser_LDADD = libixion/libixion-@IXION_API_VERSION@.la \
					 $(BOOST_THREAD_LIBS) $(BOOST_PROGRAM_OPTIONS_LIBS)

ixion_sorter_SOURCES = \
	ixion_sorter.cpp \
	sort_input_parser.hpp \
	sort_input_parser.cpp

ixion_sorter_LDADD = libixion/libixion-@IXION_API_VERSION@.la \
					 $(BOOST_THREAD_LIBS) $(BOOST_PROGRAM_OPTIONS_LIBS)

TESTS = $(top_srcdir)/test/ixion-parser-test.sh
