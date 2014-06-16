ACLOCAL_AMFLAGS = -I m4

LIBTOOL_DEPS = @LIBTOOL_DEPS@
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status libtool

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = packeteer.pc

DISTCLEANFILES = _configs.sed packeteer/packeteer-config.h

SUBDIRS = \
	packeteer \
	test
