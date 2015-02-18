AM_CPPFLAGS = -I$(top_srcdir)/include -DIXION_BUILD $(MDDS_CFLAGS)

pyexec_LTLIBRARIES = ixion.la
ixion_la_SOURCES = \
	document.hpp \
	document.cpp \
	global.hpp \
	global.cpp \
	python.cpp \
	sheet.hpp \
	sheet.cpp

ixion_la_LDFLAGS = -module -avoid-version -export-symbols-regex initixion
ixion_la_CPPFLAGS = -I$(top_srcdir)/include $(PYTHON_CFLAGS)
ixion_la_LIBADD = \
	../libixion/libixion-@IXION_API_VERSION@.la \
	$(PYTHON_LIBS)

AM_TESTS_ENVIRONMENT = PYTHONPATH=.libs$${PYTHONPATH:+:$${PYTHONPATH}}; export PYTHONPATH;
TESTS = ../../test/ixion-python-test.py
