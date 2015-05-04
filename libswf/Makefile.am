AM_CFLAGS = -std=gnu99 -Wall -Wextra -Wno-sign-compare -Wno-unused-parameter \
            -Werror-implicit-function-declaration -Wstrict-prototypes        \
            -Wpointer-arith -Wredundant-decls -D_7ZIP_ST

LIBSWF_LT_CURRENT = 5
LIBSWF_LT_REVISION = 0
LIBSWF_LT_AGE = 0

lib_LTLIBRARIES = libswf.la
libswf_la_SOURCES = parser.c parser.h swf.c swf.h internal.h
libswf_la_SOURCES += lzma/LzFind.c lzma/LzmaDec.c lzma/LzmaEnc.c

libswf_la_LDFLAGS = -no-undefined -version-info $(LIBSWF_LT_CURRENT):$(LIBSWF_LT_REVISION):$(LIBSWF_LT_AGE)
libswf_la_LDFLAGS += -export-symbols-regex '^swf_.*'

swfheadersdir = $(includedir)
dist_swfheaders_HEADERS = swf.h

EXTRA_DIST = libswf.sym
