include $(top_srcdir)/common.mk

SUBDIRS = .

lib_LTLIBRARIES = libpacketeer.la

pkginclude_HEADERS = \
	packeteer.h \
	packeteer-config.h \
	version.h \
	error.h \
	macros.h \
	types.h \
	concurrent_queue.h \
	callback.h \
	pipe.h \
	events.h \
	scheduler.h \
	connector.h \
	net/socket_address.h \
	net/network.h \
	net/detail/sysincludes.h

noinst_HEADERS = \
	detail/scheduler_impl.h \
	detail/callbacks_io.h \
	detail/callbacks_scheduled.h \
	detail/callbacks_user_defined.h \
	detail/io.h \
	detail/worker.h \
	net/detail/cidr.h

libpacketeer_la_CPPFLAGS = $(DEPS_CFLAGS)
libpacketeer_la_LIBADD = $(DEPS_LIBS)
libpacketeer_la_LDFLAGS = -version-info 1:0:0

libpacketeer_la_SOURCES = \
	version.cpp \
	error.cpp \
	packeteer.cpp \
	pipe.cpp \
	scheduler.cpp \
	connector.cpp \
	detail/worker.cpp \
	detail/scheduler_impl.cpp \
	net/socket_address.cpp \
	net/network.cpp \
	net/detail/cidr.cpp

if HAVE_EPOLL_CREATE1
libpacketeer_la_SOURCES += \
	detail/io_epoll.cpp
noinst_HEADERS += \
	detail/io_epoll.h
endif

if HAVE_SELECT
libpacketeer_la_SOURCES += \
	detail/io_select.cpp
noinst_HEADERS += \
	detail/io_select.h
endif

if HAVE_POLL
libpacketeer_la_SOURCES += \
	detail/io_poll.cpp
noinst_HEADERS += \
	detail/io_poll.h
endif
