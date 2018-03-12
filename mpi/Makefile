EXES=counter dissemination tournament
EXESFP=$(patsubst %, $(EXEDIR)/%, $(EXES))
PREFIX=gtmpi_


CC=mpicc
CCX=mpiCC

DEPFLAGS=-M

CFLAGS=-c -g -Wall -Wextra -Werror \
-Wno-unused-parameter -Wno-unused-result -Wno-unused-variable -Wno-unused-but-set-variable \
-Wconversion \
-fopenmp -DLEVEL1_DCACHE_LINESIZE=`getconf LEVEL1_DCACHE_LINESIZE` \

CPPFLAGS=$(CFLAGS)
CPPFLAGS+=-std=c++14

LDFLAGS=-lboost_system -lpthread -lgomp -lstdc++



HIGH_OPTIMIZE=0

ifeq ($(DEBUG), 0)
	HIGH_OPTIMIZE=1
endif

ifndef DEBUG
	HIGH_OPTIMIZE=1
endif

ifneq ($(HIGH_OPTIMIZE), 0)
	CFLAGS+=-O2 -flto
	CPPFLAGS+=-O2 -flto
	LDFLAGS+=-O2 -flto
endif



ROOTDIR=.
BUILDDIR=$(ROOTDIR)/work
DEPDIR=$(BUILDDIR)/dep
OBJDIR=$(BUILDDIR)/obj
EXEDIR=$(ROOTDIR)

SOURCES=$(wildcard *.cpp *.c)
DEPS=$(addsuffix .d, $(SOURCES))
OBJS=$(addsuffix .o, $(SOURCES))
DEPSFP=$(patsubst %, $(DEPDIR)/%, $(DEPS))
OBJSFP=$(patsubst %, $(OBJDIR)/%, $(OBJS))


$(shell mkdir -p $(DEPDIR) > /dev/null)
$(shell mkdir -p $(OBJDIR) > /dev/null)
$(shell mkdir -p $(EXEDIR) > /dev/null)


.PHONY: exe
exe: $(EXESFP)

.PHONY: obj
obj: $(OBJSFP)

.PHONY: dep
dep: $(DEPSFP)

.PHONY: clean
clean:
	rm -rf $(BUILDDIR)
	rm -rf $(EXESFP)

OBJS_NO_GTMP=$(shell echo $(OBJS) | tr " " "\n" | grep -Pv "^$(PREFIX)" )
OBJSFP_NO_GTMP=$(patsubst %, $(OBJDIR)/%, $(OBJS_NO_GTMP))


$(EXEDIR)/%: $(OBJSFP_NO_GTMP) $(OBJDIR)/$(PREFIX)%.cpp.o
	$(CCX) $^ -o $@ $(LDFLAGS)


-include $(DEPSFP)
$(OBJDIR)/%.cpp.o: %.cpp $(DEPDIR)/%.cpp.d
	$(CCX) $(CPPFLAGS) $< -o $@
.PRECIOUS: $(OBJDIR)/%.cpp.o

-include $(DEPSFP)
$(OBJDIR)/%.c.o: %.c $(DEPDIR)/%.c.d
	$(CC) $(CFLAGS) $< -o $@
.PRECIOUS: $(OBJDIR)/%.c.o


$(DEPDIR)/%.cpp.d: %.cpp
	@set -e; rm -f $@; \
	$(CCX) $(DEPFLAGS) $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*, $(OBJDIR)/$(@F:.d=.o) $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$
.PRECIOUS: $(DEPDIR)/%.cpp.d

$(DEPDIR)/%.c.d: %.c
	@set -e; rm -f $@; \
	$(CC) $(DEPFLAGS) $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*, $(OBJDIR)/$(@F:.d=.o) $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$
.PRECIOUS: $(DEPDIR)/%.c.d


