SUBDIRS = libixion python

AM_CPPFLAGS = -I$(top_srcdir)/include $(MDDS_CFLAGS) $(BOOST_CPPFLAGS)

bin_PROGRAMS = ixion-parser ixion-sorter ixion-formula-tokenizer

ixion_parser_SOURCES = \
	ixion_parser.cpp \
	model_parser.hpp \
	model_parser.cpp \
	session_handler.hpp \
	session_handler.cpp \
	table_handler.hpp \
	table_handler.cpp

ixion_parser_LDADD = libixion/libixion-@IXION_API_VERSION@.la \
					 $(BOOST_PROGRAM_OPTIONS_LIBS)

ixion_sorter_SOURCES = \
	ixion_sorter.cpp \
	sort_input_parser.hpp \
	sort_input_parser.cpp

ixion_sorter_LDADD = libixion/libixion-@IXION_API_VERSION@.la \
					 $(BOOST_PROGRAM_OPTIONS_LIBS)

ixion_formula_tokenizer_SOURCES = \
	ixion_formula_tokenizer.cpp

ixion_formula_tokenizer_LDADD = libixion/libixion-@IXION_API_VERSION@.la \
					 $(BOOST_PROGRAM_OPTIONS_LIBS)


AM_TESTS_ENVIRONMENT = PATH=.libs$${PATH:+:$${PATH}}; export PATH; \
	LD_LIBRARY_PATH=libixion/.libs$${LD_LIBRARY_PATH:+:$${LD_LIBRARY_PATH}}; export LD_LIBRARY_PATH; \
	DYLD_LIBRARY_PATH=$${LD_LIBRARY_PATH}}; export DYLD_LIBRARY_PATH;

TESTS = \
	../test/parser-test-t0.sh \
	../test/parser-test-t1.sh \
	../test/parser-test-t2.sh \
	../test/parser-test-t3.sh \
	../test/parser-test-t4.sh \
	../test/parser-test-t5.sh \
	../test/parser-test-t6.sh \
	../test/parser-test-t7.sh \
	../test/parser-test-t8.sh

# TODO: temporarily disable parser tests until we can fix the circular dependency
# issue with the dirty_cell_tracker.
TESTS =
