AM_CFLAGS = -Wall

noinst_PROGRAMS = test
test_SOURCES = test.c
test_CPPFLAGS = -I$(top_srcdir)/libswf
test_LDADD = $(top_builddir)/libswf/.libs/libswf.a
test_LDFLAGS = $(AM_LDFLAGS) -static
