# $Id:$
AUTOMAKE_OPTIONS = no-dependencies

bin_PROGRAMS = arpsweep
man_MANS = arpsweep.8

arpsweep_SOURCES = arpsweep.c

arpsweep_LDADD = @LIBOBJS@ -lpcap -lnet

EXTRA_DIST = $(man_MANS) arpsweep.spec
