
pyexec_LTLIBRARIES = ixion.la
ixion_la_SOURCES = \
	python.cpp

ixion_la_LDFLAGS = -module -avoid-version -export-symbols-regex initixion
ixion_la_CPPFLAGS = -I$(top_srcdir)/include $(PYTHON_CFLAGS)
ixion_la_LIBADD = $(PYTHON_LIBS)

