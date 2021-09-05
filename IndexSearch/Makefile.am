# Process this file with automake to produce Makefile.in

pkginclude_HEADERS = \
	DBusIndex.h \
	FieldMapperInterface.h \
	FilterWrapper.h \
	IndexInterface.h \
	ModuleFactory.h \
	ModuleProperties.h \
	OpenSearchParser.h \
	PinotDBus_proxy.h \
	PluginParsers.h \
	PluginWebEngine.h \
	QueryProperties.h \
	ResultsExporter.h \
	SearchEngineInterface.h \
	SearchPluginProperties.h \
	SherlockParser.h \
	WebEngine.h

nobase_pkginclude_HEADERS = \
	cjkv/CJKVTokenizer.h

if HAVE_DBUS
pkglib_LTLIBRARIES = libIndex.la libIndexSearch.la
else
pkglib_LTLIBRARIES = libIndexSearch.la
endif

libIndex_la_LDFLAGS = \
	-static

libIndex_la_SOURCES = \
	PinotDBus_proxy.cpp \
	DBusIndex.cpp

libIndexSearch_la_LDFLAGS = \
	-static

libIndexSearch_la_SOURCES = \
	FilterWrapper.cpp \
	ModuleFactory.cpp \
	OpenSearchParser.cpp \
	PluginWebEngine.cpp \
	QueryProperties.cpp \
	ResultsExporter.cpp \
	SearchEngineInterface.cpp \
	SearchPluginProperties.cpp \
	WebEngine.cpp

if HAVE_DBUS
libIndexSearch_la_SOURCES += PinotDBus_proxy.cpp DBusIndex.cpp
endif

if HAVE_BOOST_SPIRIT
libIndexSearch_la_SOURCES += SherlockParser.cpp
endif

if HAVE_DBUS
bin_PROGRAMS = pinot-label
endif

pinot_label_LDFLAGS = \
	-export-dynamic

pinot_label_LDADD = \
	-L$(top_builddir)/Utils \
	-lIndex -lUtils -lBasicUtils \
	@GLIBMM_LIBS@ @GIOMM_LIBS@ @DBUS_LIBS@ \
	@GTHREAD_LIBS@ @HTTP_LIBS@ @MISC_LIBS@

pinot_label_SOURCES = pinot-label.cpp

pinot_label_DEPENDENCIES = libIndex.la

AM_CXXFLAGS = \
	@MISC_CFLAGS@ \
	-I$(top_srcdir)/Utils \
	-I$(top_srcdir)/Tokenize \
	-I$(top_srcdir)/Tokenize/filters \
	-I$(top_srcdir)/Collect \
	-I$(top_srcdir)/IndexSearch/cjkv \
	@HTTP_CFLAGS@ @XML_CFLAGS@ \
	@INDEX_CFLAGS@ @GIOMM_CFLAGS@ @GLIBMM_CFLAGS@

if HAVE_DBUS
AM_CXXFLAGS += @DBUS_CFLAGS@ \
	-DHAVE_DBUS
endif

if HAVE_BOOST_SPIRIT
AM_CXXFLAGS += -DHAVE_BOOST_SPIRIT
endif

