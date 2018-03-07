EXES=counter mcs tree
EXESFP=$(patsubst %, $(EXEDIR)/%, $(EXES))
PREFIX=gtmp_


CC=g++

DEPFLAGS=-M
CPPFLAGS=-c -g -std=c++14 -Wall -Wextra -Werror \
-Wno-unused-parameter -Wno-unused-result -Wno-unused-variable -Wno-unused-but-set-variable \
-Wconversion \
-fopenmp -DLEVEL1_DCACHE_LINESIZE=`getconf LEVEL1_DCACHE_LINESIZE`
LDFLAGS=-lboost_system -lpthread -lgomp



HIGH_OPTIMIZE=0

ifeq ($(DEBUG), 0)
	HIGH_OPTIMIZE=1
endif

ifndef DEBUG
	HIGH_OPTIMIZE=1
endif

ifneq ($(HIGH_OPTIMIZE), 0)
	CPPFLAGS+=-O2 -flto
	LDFLAGS+=-O2 -flto
endif



ROOTDIR=.
BUILDDIR=$(ROOTDIR)/work
DEPDIR=$(BUILDDIR)/dep
OBJDIR=$(BUILDDIR)/obj
EXEDIR=$(ROOTDIR)

SOURCES=$(wildcard *.cpp)
DEPS=$(SOURCES:.cpp=.d)
OBJS=$(SOURCES:.cpp=.o)
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
	rm -rf $(DEPSFP) \
	rm -rf $(OBJSFP) \
	rm -rf $(EXESFP)

OBJS_NO_GTMP := $(shell echo $(OBJS) | tr " " "\n" | grep -Pv "^$(PREFIX)" )
OBJSFP_NO_GTMP=$(patsubst %, $(OBJDIR)/%, $(OBJS_NO_GTMP))


$(EXEDIR)/%: $(OBJSFP_NO_GTMP) $(OBJDIR)/$(PREFIX)%.o
	$(CC) $^ -o $@ $(LDFLAGS)


-include $(DEPSFP)
$(OBJDIR)/%.o: %.cpp $(DEPDIR)/%.d
	$(CC) $(CPPFLAGS) $< -o $@
.PRECIOUS: $(OBJDIR)/%.o


$(DEPDIR)/%.d: %.cpp
	@set -e; rm -f $@; \
	$(CC) $(DEPFLAGS) $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*, $(OBJDIR)/$(@F:.d=.o) $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$
.PRECIOUS: $(DEPDIR)/%.d


