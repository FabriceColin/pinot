# Process this file with automake to produce Makefile.in

pkginclude_HEADERS = \
	ActionQueue.h \
	CrawlHistory.h \
	MetaDataBackup.h \
	QueryHistory.h \
	SQLDB.h \
	SQLiteBase.h \
	ViewHistory.h

pkglib_LTLIBRARIES = libSQL.la libSQLite.la libSQLDB.la

libSQL_la_LDFLAGS = \
	-static

libSQL_la_SOURCES = \
	SQLDB.cpp

libSQLite_la_LDFLAGS = \
	-static

libSQLite_la_SOURCES = \
	SQLiteBase.cpp

libSQLDB_la_LDFLAGS = \
	-static

libSQLDB_la_SOURCES = \
	ActionQueue.cpp \
	CrawlHistory.cpp \
	MetaDataBackup.cpp \
	QueryHistory.cpp \
	ViewHistory.cpp

AM_CXXFLAGS = \
	@MISC_CFLAGS@ \
	-I$(top_srcdir)/Utils

