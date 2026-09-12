
AM_CPPFLAGS = \
	-I$(top_srcdir)/include \
	$(MDDS_CFLAGS) $(LIBIXION_CFLAGS) \
	-DSRCDIR=\""$(top_srcdir)"\"

bin_PROGRAMS =

EXTRA_PROGRAMS = \
	document-simple \
	model-context-simple \
	cell-access \
	sheet-view \
	formula-syntax \
	names-and-tables \
	sheet-copy

document_simple_SOURCES = document_simple.cpp
document_simple_LDADD = ../src/libixion/libixion-@IXION_API_VERSION@.la

model_context_simple_SOURCES = model_context_simple.cpp
model_context_simple_LDADD = ../src/libixion/libixion-@IXION_API_VERSION@.la

cell_access_SOURCES = cell_access.cpp
cell_access_LDADD = ../src/libixion/libixion-@IXION_API_VERSION@.la

sheet_view_SOURCES = sheet_view.cpp
sheet_view_LDADD = ../src/libixion/libixion-@IXION_API_VERSION@.la

formula_syntax_SOURCES = formula_syntax.cpp
formula_syntax_LDADD = ../src/libixion/libixion-@IXION_API_VERSION@.la

names_and_tables_SOURCES = names_and_tables.cpp
names_and_tables_LDADD = ../src/libixion/libixion-@IXION_API_VERSION@.la

sheet_copy_SOURCES = sheet_copy.cpp
sheet_copy_LDADD = ../src/libixion/libixion-@IXION_API_VERSION@.la

TESTS = \
	document-simple \
	model-context-simple \
	cell-access \
	sheet-view \
	formula-syntax \
	names-and-tables \
	sheet-copy

distclean-local:
	rm -rf $(TESTS)
