include $(top_srcdir)/common.mk

noinst_LTLIBRARIES = libpacketflinger_detail.la

libpacketflinger_detail_la_CPPFLAGS = $(DEPS_CFLAGS)
libpacketflinger_detail_la_LIBADD = $(DEPS_LIBS)

libpacketflinger_detail_la_SOURCES = \
	worker.cpp \
	scheduler_impl.cpp

if HAVE_EPOLL_CREATE1
# FIXME add implementation for handling FDs with epoll
endif

if HAVE_SELECT
# FIXME add implementation for handling FDs with select
endif

noinst_HEADERS = \
	scheduler_impl.h \
	callbacks_io.h \
	callbacks_scheduled.h \
	callbacks_user_defined.h \
	worker.h
