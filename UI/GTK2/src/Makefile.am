# Process this file with automake to produce Makefile.in

noinst_HEADERS = \
	importDialog_glade.hh \
	importDialog.hh \
	indexDialog_glade.hh \
	indexDialog.hh \
	launcherDialog.hh \
	launcherDialog_glade.hh \
	mainWindow_glade.hh \
	mainWindow.hh \
	prefsWindow_glade.hh \
	prefsWindow.hh \
	propertiesDialog_glade.hh \
	propertiesDialog.hh \
	queryDialog_glade.hh \
	queryDialog.hh \
	statisticsDialog_glade.hh \
	statisticsDialog.hh \
	EnginesTree.hh \
	IndexPage.hh \
	ModelColumns.hh \
	Notebook.hh \
	PinotUtils.hh \
	ResultsTree.hh \
	UIThreads.hh

pinot_SOURCES = \
	pinot.cc \
	importDialog_glade.cc \
	importDialog.cc \
	indexDialog_glade.cc \
	indexDialog.cc \
	launcherDialog.cc \
	launcherDialog_glade.cc \
	mainWindow_glade.cc \
	mainWindow.cc \
	prefsWindow_glade.cc \
	prefsWindow.cc \
	propertiesDialog_glade.cc \
	propertiesDialog.cc \
	queryDialog_glade.cc \
	queryDialog.cc \
	statisticsDialog_glade.cc \
	statisticsDialog.cc \
	EnginesTree.cc \
	IndexPage.cc \
	ModelColumns.cc \
	Notebook.cc \
	PinotUtils.cc \
	ResultsTree.cc \
	UIThreads.cc

bin_PROGRAMS = pinot

AM_CXXFLAGS = \
	@MISC_CFLAGS@ \
	-I$(top_srcdir)/Utils \
	-I$(top_srcdir)/Tokenize \
	-I$(top_srcdir)/Tokenize/filters \
	-I$(top_srcdir)/SQL \
	-I$(top_srcdir)/Monitor \
	-I$(top_srcdir)/Collect \
	-I$(top_srcdir)/IndexSearch \
	-I$(top_srcdir)/Core \
	@SQL_CFLAGS@ @HTTP_CFLAGS@ @XML_CFLAGS@ @DBUS_CFLAGS@ \
	@INDEX_CFLAGS@ @GTHREAD_CFLAGS@ @GIOMM_CFLAGS@ @GTKMM_CFLAGS@

if HAVE_DBUS
AM_CXXFLAGS += -DHAVE_DBUS
endif

pinot_LDFLAGS = \
	-export-dynamic

pinot_LDADD = \
	-L$(top_builddir)/Utils \
	-L$(top_builddir)/Tokenize \
	-L$(top_builddir)/SQL \
	-L$(top_builddir)/Monitor \
	-L$(top_builddir)/Collect \
	-L$(top_builddir)/IndexSearch \
	-L$(top_srcdir)/Core \
	-lCore -lThread -lIndexSearch -lMonitor -lCollect -lSQLDB -lSQLite -lSQL \
	-lTokenize -lFilter -lBasicUtils -lUtils @LIBS@ \
	@GTKMM_LIBS@ @GIOMM_LIBS@ @DBUS_LIBS@ @GTHREAD_LIBS@ @XML_LIBS@ \
	@HTTP_LIBS@ @SQL_LIBS@ @MISC_LIBS@

