include $(top_srcdir)/common.mk

TESTS = testsuite
check_PROGRAMS = $(TESTS)

testsuite_CPPFLAGS = $(CPPUNIT_CFLAGS)
testsuite_LDADD = $(top_builddir)/packeteer/libpacketeer.la \
	$(CPPUNIT_LIBS) $(TWINE_LIBS)

testsuite_SOURCES = \
	test_callback.cpp \
	test_concurrent_queue.cpp \
	test_pipe.cpp \
	test_scheduler_impl.cpp \
	test_scheduler.cpp \
	testsuite.cpp
