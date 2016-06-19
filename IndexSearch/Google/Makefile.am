# Process this file with automake to produce Makefile.in

noinst_HEADERS = \
	GAPI.nsmap \
	GAPIGoogleSearchBindingProxy.h \
	GAPIH.h \
	GAPIStub.h \
	GoogleAPIEngine.h \
	GoogleSearch.h \
	GAPIClientLib.cpp

if HAVE_SOAP
lib_LTLIBRARIES = libgoogleapibackend.la

libgoogleapibackend_la_SOURCES = \
	GAPIC.cpp \
	GAPIClient.cpp \
	GoogleAPIEngine.cpp \
	ModuleExports.cpp

libgoogleapibackend_la_LDFLAGS = -module -shared -nostartfiles

libgoogleapibackend_la_LIBADD = \
	-L$(top_builddir)/Utils \
	-lBasicUtils \
	@SOAP_LIBS@ @MISC_LIBS@

GoogleSearch.h :
	wsdl2h -n gapi -o GoogleSearch.h googleapi/GoogleSearch.wsdl

GAPIClientLib.cpp : GoogleSearch.h
	soapcpp2 -n -pGAPI -I/usr/include/gsoap/import -I/usr/share/gsoap/import GoogleSearch.h

libgoogleapibackend_la_CXXFLAGS = \
	-I$(top_srcdir)/Utils \
	-I$(top_srcdir)/Tokenize \
	-I$(top_srcdir)/Tokenize/filters \
	-I$(top_srcdir)/Collect \
	-I$(top_srcdir)/IndexSearch \
	@HTTP_CFLAGS@ @XML_CFLAGS@ @SOAP_CFLAGS@
else
noinst_HEADERS += \
	GAPIClientLib.cpp \
	GAPIC.cpp \
	GAPIClient.cpp \
	GoogleAPIEngine.cpp \
	ModuleExports.cpp
endif

