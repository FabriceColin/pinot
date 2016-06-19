# Process this file with automake to produce Makefile.in

noinst_HEADERS = \
	XesamEngine.h

if HAVE_XESAM_GLIB
lib_LTLIBRARIES = libxesamglibbackend.la

libxesamglibbackend_la_SOURCES = \
	ModuleExports.cpp \
	XesamEngine.cpp

libxesamglibbackend_la_LDFLAGS = -module -shared -nostartfiles

libxesamglibbackend_la_LIBADD = \
	-L$(top_builddir)/Utils \
	-lBasicUtils \
	@XESAMGLIB_LIBS@ @MISC_LIBS@

AM_CXXFLAGS = \
	@MISC_CFLAGS@ \
	-I$(top_srcdir)/Utils \
	-I$(top_srcdir)/Tokenize \
	-I$(top_srcdir)/Tokenize/filters \
	-I$(top_srcdir)/Collect \
	-I$(top_srcdir)/IndexSearch \
	@HTTP_CFLAGS@ @XML_CFLAGS@ @INDEX_CFLAGS@ @GLIBMM_CFLAGS@ @XESAMGLIB_CFLAGS@
endif

