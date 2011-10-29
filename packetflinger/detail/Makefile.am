include $(top_srcdir)/common.mk

noinst_LTLIBRARIES = libpacketflinger_detail.la

# FIXME
libpacketflinger_detail_la_CPPFLAGS = $(DEPS_CFLAGS)
libpacketflinger_detail_la_LIBADD = $(DEPS_LIBS)

libpacketflinger_detail_la_SOURCES = \
	worker.cpp \
	scheduler_impl_generic.cpp

if USE_EPOLL
libpacketflinger_detail_la_SOURCES += \
	scheduler_impl_epoll.cpp
endif

noinst_HEADERS = \
	scheduler_impl.h \
	worker.h
