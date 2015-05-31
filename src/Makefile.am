SUBDIRS = libixion
if PYTHON
	SUBDIRS += python
endif

AM_CPPFLAGS = -I$(top_srcdir)/include $(MDDS_CFLAGS)

bin_PROGRAMS = ixion-parser ixion-sorter

ixion_parser_SOURCES = \
	ixion_parser.cpp \
	model_parser.hpp \
	model_parser.cpp \
	session_handler.hpp \
	session_handler.cpp \
	table_handler.hpp \
	table_handler.cpp

ixion_parser_LDADD = libixion/libixion-@IXION_API_VERSION@.la \
					 $(BOOST_THREAD_LIBS) $(BOOST_PROGRAM_OPTIONS_LIBS)

ixion_sorter_SOURCES = \
	ixion_sorter.cpp \
	sort_input_parser.hpp \
	sort_input_parser.cpp

ixion_sorter_LDADD = libixion/libixion-@IXION_API_VERSION@.la \
					 $(BOOST_THREAD_LIBS) $(BOOST_PROGRAM_OPTIONS_LIBS)

AM_TESTS_ENVIRONMENT = PATH=.libs$${PATH:+:$${PATH}}; epxort PATH; \
	LD_LIBRARY_PATH=libixion/.libs$${LD_LIBRARY_PATH:+:$${LD_LIBRARY_PATH}}; export LD_LIBRARY_PATH;
TESTS = ../test/ixion-parser-test.sh
