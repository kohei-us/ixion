
AM_CPPFLAGS = \
	-I$(top_srcdir)/include \
	$(MDDS_CFLAGS) $(LIBIXION_CFLAGS) \
	-DSRCDIR=\""$(top_srcdir)"\"

bin_PROGRAMS =

EXTRA_PROGRAMS = \
	cell-access

cell_access_SOURCES = cell_access.cpp
cell_access_LDADD = ../../src/libixion/libixion-@IXION_API_VERSION@.la

TESTS = \
	cell-access

distclean-local:
	rm -rf $(TESTS)
