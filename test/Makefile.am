include $(top_srcdir)/common.mk

TESTS = testsuite
check_PROGRAMS = $(TESTS)

testsuite_CPPFLAGS = $(CPPUNIT_CFLAGS) $(BOOST_CPPFLAGS)
testsuite_LDADD = $(top_builddir)/packetflinger/libpacketflinger.la \
	$(CPPUNIT_LIBS) $(BOOST_THREAD_LIB)

testsuite_SOURCES = \
	test_duration.cpp \
	testsuite.cpp
