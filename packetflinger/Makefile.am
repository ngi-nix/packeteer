AM_CXXFLAGS = -I$(top_srcdir) @AM_CXXFLAGS@

lib_LTLIBRARIES = libpacketflinger.la

libpacketflinger_la_CPPFLAGS = $(DEPS_CFLAGS)
libpacketflinger_la_LIBADD = $(DEPS_LIBS)
libpacketflinger_la_LDFLAGS = -version-info 1:0:0

libpacketflinger_la_libincludedir = $(libdir)/packetflinger/include
nodist_libpacketflinger_la_libinclude_HEADERS = \
	packetflinger-config.h

libpacketflinger_la_SOURCES = \
	error.cpp \
	version.cpp \
	packetflinger.cpp

libpacketflinger_la_includedir = $(includedir)/packetflinger/include
libpacketflinger_la_include_HEADERS = \
	error.h \
	macros.h \
	version.h \
	packetflinger.h \
	shared_ptr.h
