include $(top_srcdir)/common.mk

SUBDIRS = .

lib_LTLIBRARIES = libpacketflinger.la

pkginclude_HEADERS = \
	packetflinger.h \
	packetflinger-config.h \
	version.h \
	error.h \
	macros.h \
	types.h \
	concurrent_queue.h \
	callback.h \
	pipe.h \
	events.h \
	scheduler.h

noinst_HEADERS = \
	detail/scheduler_impl.h \
	detail/callbacks_io.h \
	detail/callbacks_scheduled.h \
	detail/callbacks_user_defined.h \
	detail/io.h \
	detail/worker.h

libpacketflinger_la_CPPFLAGS = $(DEPS_CFLAGS)
libpacketflinger_la_LIBADD = $(DEPS_LIBS)
libpacketflinger_la_LDFLAGS = -version-info 1:0:0

libpacketflinger_la_SOURCES = \
	version.cpp \
	error.cpp \
	packetflinger.cpp \
	pipe.cpp \
	scheduler.cpp \
	detail/worker.cpp \
	detail/scheduler_impl.cpp

if HAVE_EPOLL_CREATE1
libpacketflinger_la_SOURCES += \
	detail/io_epoll.cpp
noinst_HEADERS += \
	detail/io_epoll.h
endif

if HAVE_SELECT
libpacketflinger_la_SOURCES += \
	detail/io_select.cpp
noinst_HEADERS += \
	detail/io_select.h
endif
