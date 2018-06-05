CFLAGS := -g -W -Wall -Wno-unused-parameter -fPIC -I. -DGMP -O$(O)

ifneq (,$(filter $(shell uname -n),elaines-MacBook-Pro.local Nicholass-Air-5.local))
    LIBS := -L/usr/local/lib
    CFLAGS += -I/usr/local/include
else
    LIBS :=
endif

LIBS += -L/usr/local/lib -lgmpxx -lgmp

ifneq ($(shell uname -n),beepboop)
    LDFLAGS :=
    CFLAGS += -fno-omit-frame-pointer
    ifneq ($(shell uname -r),15.6.0)
        LIBS += -lprofiler -ltcmalloc
    endif
else
    LDFLAGS :=
endif

ifneq ($(shell uname),Darwin)
    LIBS += -lbsd
endif

CC = gcc
CXX = g++ -std=c++11

DEPSDIR := .deps

CLEAN = corels libcorels.so *~ *.o *.so

all: corels libcorels.so

corels: main.o corels.o pmap.o cache.o rulelib.o utils.o
	$(CXX) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)

libcorels.so: run.o params.o corels.o pmap.o cache.o rulelib.o utils.o
	$(CXX) -shared  -Wl,-soname,$@ -o $@ $^

run.o: run.cc $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -c $< -o $@

main.o: main.cc $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -c $< -o $@

corels.o: corels.cc $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -c $< -o $@

pmap.o: pmap.cc $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -c $< -o $@

utils.o: utils.cc $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -c $< -o $@

cache.o: cache.cc $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -c $< -o $@

rulelib.o: rulelib.c $(DEPSDIR)/stamp
	$(CC) $(CFLAGS) $(DEPCFLAGS) -c $< -o $@

params.o: params.c $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -c $< -o $@

$(DEPSDIR)/stamp:
	mkdir -p $(dir $@)
	touch $@

clean:
	$(RM) $(wildcard $(CLEAN))
	$(RM) -r $(DEPSDIR)

DEPFILES := $(wildcard $(DEPSDIR)/*.d)
ifneq ($(DEPFILES),)
include $(DEPFILES)
endif

.PHONY: all clean
.SUFFIXES:
