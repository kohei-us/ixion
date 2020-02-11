
AM_CPPFLAGS = \
	-I$(top_srcdir)/include \
	$(MDDS_CFLAGS) $(LIBIXION_CFLAGS) \
	-DSRCDIR=\""$(top_srcdir)"\"

bin_PROGRAMS =

EXTRA_PROGRAMS = \
	model-context-simple

model_context_simple_SOURCES = model_context_simple.cpp
model_context_simple_LDADD = ../src/libixion/libixion-@IXION_API_VERSION@.la

TESTS = \
	model-context-simple

distclean-local:
	rm -rf $(TESTS)
