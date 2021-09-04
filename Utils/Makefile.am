# Process this file with automake to produce Makefile.in

pkginclude_HEADERS = \
	CommandLine.h \
	Document.h \
	DocumentInfo.h \
	Languages.h \
	MIMEScanner.h \
	Memory.h \
	NLS.h \
	PinotDBus_common.h \
	StringManip.h \
	TimeConverter.h \
	Timer.h \
	Url.h \
	Visibility.h

noinst_HEADERS = \
	xdgmime/xdgmimealias.h \
	xdgmime/xdgmimecache.h \
	xdgmime/xdgmimeglob.h \
	xdgmime/xdgmime.h \
	xdgmime/xdgmimeint.h \
	xdgmime/xdgmimemagic.h \
	xdgmime/xdgmimeparent.h

if HAVE_GIO_MIME
noinst_HEADERS += \
	xdgmime/xdgmimealias.c \
	xdgmime/xdgmime.c \
	xdgmime/xdgmimecache.c \
	xdgmime/xdgmimeglob.c \
	xdgmime/xdgmimeint.c \
	xdgmime/xdgmimemagic.c \
	xdgmime/xdgmimeparent.c
endif

pkglib_LTLIBRARIES = libBasicUtils.la libUtils.la

libBasicUtils_la_LDFLAGS = \
	-static

libBasicUtils_la_SOURCES = \
	CommandLine.cpp \
	Document.cpp \
	DocumentInfo.cpp \
	StringManip.cpp \
	TimeConverter.cpp \
	Timer.cpp \
	Url.cpp

libUtils_la_LDFLAGS = \
	-static

libUtils_la_SOURCES = \
	Languages.cpp \
	MIMEScanner.cpp \
	PinotDBus_common.cpp \
	Memory.cpp

if HAVE_GIO_MIME
else
libUtils_la_SOURCES += \
	xdgmime/xdgmimealias.c \
	xdgmime/xdgmime.c \
	xdgmime/xdgmimecache.c \
	xdgmime/xdgmimeglob.c \
	xdgmime/xdgmimeint.c \
	xdgmime/xdgmimemagic.c \
	xdgmime/xdgmimeparent.c
endif

AM_CFLAGS = -fPIC -DXDG_PREFIX=pinot_xdg -DUNAC_VERSION=\"1.0.7\"
AM_CXXFLAGS = \
	@MISC_CFLAGS@ \
	@HTTP_CFLAGS@ @INDEX_CFLAGS@ \
	@GIOMM_CFLAGS@ @GLIBMM_CFLAGS@ -DXDG_PREFIX=pinot_xdg -DUNAC_VERSION=\"1.0.7\"

