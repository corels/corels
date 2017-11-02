AUTOMAKE_OPTIONS = foreign
MAJOR_VERSION=1
MINOR_VERSION=0
VERSION=$(MAJOR_VERSION).$(MINOR_VERSION)
LIBTOOL_VERSION=$(MAJOR_VERSION):$(MINOR_VERSION)

ACLOCAL_AMFLAGS= -I m4
AM_CPPFLAGS = -DGMP -Wall -I. -I/usr/local/include -I/opt/local/include $(INCLUDES)
AM_CXXFLAGS = -DGMP -Wall -I. -I/usr/local/include -I/opt/local/include $(INCLUDES)
AM_LDFLAGS = -L/opt/local/lib -L/usr/local/lib -lgsl -lgmp -lm -lc

# Support both static and dynamic builds
bin_PROGRAMS = corels
# ADD THIS TO PREVIOUS LINE FOR STATIC BUILD: corels_static
include_HEADERS = rule.h queue.hh pmap.hh alloc.hh cache.hh utils.hh
dist_pkgdata_DATA = data/compas_test-binary.csv data/compas_test.csv data/compas_test.label data/compas_test.out data/compas_train-binary.csv data/compas_train.csv data/compas_train.label data/compas_train.minor data/compas_train.out

# Static build -- not built by default, but it works
corels_static_SOURCES = main.cc queue.hh pmap.hh alloc.hh cache.hh utils.hh
corels_static_LDADD = libscorels.la
corels_static_LDFLAGS = -static

# Dynamic build
corels_SOURCES = main.cc queue.hh pmap.hh alloc.hh cache.hh utils.hh
corels_LDADD = libcorels.la

lib_LTLIBRARIES = libcorels.la
libcorels_la_SOURCES = alloc.hh cache.cc cache.hh corels.cc pmap.cc pmap.hh queue.hh rule.h rulelib.c utils.cc utils.hh
libcorels_la_LDFLAGS = -version-info $(LIBTOOL_VERSION)


