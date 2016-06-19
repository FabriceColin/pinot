# Process this file with automake to produce Makefile.in

pkginclude_HEADERS = \
	INotifyMonitor.h \
	MonitorEvent.h \
	MonitorFactory.h \
	MonitorHandler.h \
	MonitorInterface.h

pkglib_LTLIBRARIES = libMonitor.la

libMonitor_la_LDFLAGS = \
	-static

libMonitor_la_SOURCES = \
	MonitorEvent.cpp \
	MonitorFactory.cpp \
	MonitorHandler.cpp

if HAVE_LINUX_INOTIFY
libMonitor_la_SOURCES += INotifyMonitor.cpp
endif

libMonitor_la_CXXFLAGS = \
	@MISC_CFLAGS@ \
	-I$(top_srcdir)/Utils \
	-I$(top_srcdir)/Tokenize \
	-I$(top_srcdir)/Tokenize/filters \
	-I$(top_srcdir)/SQL \
	-I$(top_srcdir)/Collect \
	-I$(top_srcdir)/Index \
	-I$(top_srcdir)/Search \
	@HTTP_CFLAGS@ @XML_CFLAGS@ @INDEX_CFLAGS@ \
	@GMIME_CFLAGS@ @SIGCPP_CFLAGS@

if HAVE_LINUX_INOTIFY
libMonitor_la_CXXFLAGS += -DHAVE_LINUX_INOTIFY
endif

