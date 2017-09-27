## Process this file with automake to produce Makefile.in
# Makefile.am for ctapi-mkt
# Andreas Tille <tille@debian.org>
# GPL

EXTRA_DIST = README

lib_LTLIBRARIES  = libctapimkt.la

libctapimkt_la_LDFLAGS = $(AM_LDFLAGS) -version-info @GENERIC_LIBRARY_VERSION@

libctapimkt_la_SOURCES = ctapi.c ctapimkt/ctapi.h

bin_PROGRAMS = mktlin
mktlin_SOURCES = mktlin.c
mktlin_LDADD = $(LDADD)
LDADD = -lrt -lusb -ldl -lctapimkt

MAINTAINERCLEANFILES = autom4te.cache
