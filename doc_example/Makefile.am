
AM_CPPFLAGS = \
	-I$(top_srcdir)/include \
	$(MDDS_CFLAGS) $(LIBIXION_CFLAGS) \
	-DSRCDIR=\""$(top_srcdir)"\"

bin_PROGRAMS =

EXTRA_PROGRAMS = \
	model-context

model_context_SOURCES = model_context.cpp
model_context_LDADD = ../src/libixion/libixion-@IXION_API_VERSION@.la

TESTS = \
	model-context

distclean-local:
	rm -rf $(TESTS)
