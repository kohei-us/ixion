
AM_CPPFLAGS = \
	-I$(top_srcdir)/include \
	$(MDDS_CFLAGS) $(LIBIXION_CFLAGS) \
	-DSRCDIR=\""$(top_srcdir)"\"

bin_PROGRAMS =

EXTRA_PROGRAMS = \
	document-simple \
	model-context-simple \
	cell-access

document_simple_SOURCES = document_simple.cpp
document_simple_LDADD = ../src/libixion/libixion-@IXION_API_VERSION@.la

model_context_simple_SOURCES = model_context_simple.cpp
model_context_simple_LDADD = ../src/libixion/libixion-@IXION_API_VERSION@.la

cell_access_SOURCES = cell_access.cpp
cell_access_LDADD = ../src/libixion/libixion-@IXION_API_VERSION@.la

TESTS = \
	document-simple \
	model-context-simple \
	cell-access

distclean-local:
	rm -rf $(TESTS)
