# Process this file with automake to produce Makefile.in

noinst_HEADERS = \
	BuilderTranslations.h \
	ImportDialog.h \
	IndexDialog.h \
	LauncherDialog.h \
	MainWindow.h \
	PrefsWindow.h \
	PropertiesDialog.h \
	QueryDialog.h \
	StatisticsDialog.h \
	EnginesTree.h \
	IndexPage.h \
	ModelColumns.h \
	Notebook.h \
	PinotUtils.h \
	ResultsTree.h \
	UIThreads.h

pinot_SOURCES = \
	pinot.cpp \
	ImportDialog.cpp \
	IndexDialog.cpp \
	LauncherDialog.cpp \
	MainWindow.cpp \
	PrefsWindow.cpp \
	PropertiesDialog.cpp \
	QueryDialog.cpp \
	StatisticsDialog.cpp \
	EnginesTree.cpp \
	IndexPage.cpp \
	ModelColumns.cpp \
	Notebook.cpp \
	PinotUtils.cpp \
	ResultsTree.cpp \
	UIThreads.cpp

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
	@SQL_CFLAGS@ @HTTP_CFLAGS@ @XML_CFLAGS@ \
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
	@GTKMM_LIBS@ @GIOMM_LIBS@ @GTHREAD_LIBS@ @XML_LIBS@ \
	@HTTP_LIBS@ @SQL_LIBS@ @MISC_LIBS@

