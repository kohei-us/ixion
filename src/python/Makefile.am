if BUILD_PYTHON

pyexec_LTLIBRARIES = ixion.la
ixion_la_SOURCES = \
	document.hpp \
	document.cpp \
	global.hpp \
	global.cpp \
	python.cpp \
	sheet.hpp \
	sheet.cpp

ixion_la_LDFLAGS = -module -avoid-version -export-symbols-regex PyInit_ixion
ixion_la_CPPFLAGS = -I$(top_srcdir)/include $(PYTHON_CFLAGS) $(MDDS_CFLAGS)
ixion_la_LIBADD = \
	../libixion/libixion-@IXION_API_VERSION@.la \
	$(PYTHON_LIBS)

if OSX

TESTS = ../../bin/run-python-test-osx.sh

else

AM_TESTS_ENVIRONMENT = . test-env.sh;

TESTS = \
	../../test/python/document.py \
	../../test/python/module.py

endif

endif
