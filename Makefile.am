ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libtruepeak.la
libtruepeak_la_SOURCES = \
    src/truepeak/truepeak.h \
    src/truepeak/truepeak.c

if BUILD_FINDTRUEPEAK
bin_PROGRAMS = findtruepeak
AM_CPPFLAGS = $(DEPS_CFLAGS)
findtruepeak_LDADD = libtruepeak.la $(DEPS_LIBS)
findtruepeak_SOURCES = \
    src/findtruepeak.c
endif