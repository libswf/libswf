ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = foreign
EXTRA_DIST = libswf.pc.in Changelog

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libswf.pc

if ENABLE_TEST
    test = test
endif

SUBDIRS = libswf $(test)

