SUBDIRS = libixion .

INCLUDES = -I$(top_srcdir)/inc

bin_PROGRAMS = ixion_parser ixion_sorter
ixion_parser_SOURCES = ixion_parser.cpp
ixion_parser_LDADD = libixion/libixion-@IXION_API_VERSION@.la \
					 -lboost_thread -lboost_program_options
ixion_parser_CPPFLAGS = -Ilibixion

ixion_sorter_SOURCES = ixion_sorter.cpp
ixion_sorter_LDADD = libixion/libixion-@IXION_API_VERSION@.la \
					 -lboost_thread -lboost_program_options
ixion_sorter_CPPFLAGS = -Ilibixion
