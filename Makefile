CXX = g++
ACK = ack-grep
PERL = perl
#WARNINGS = -Wall -Weffc++ -Wshadow -Wno-non-virtual-dtor
WARNINGS = -Wall -Wshadow -Wnon-virtual-dtor -Wsign-promo -Wextra
PERLCXX := $(shell $(PERL) -MExtUtils::Embed -e ccopts)
DEBUG = -ggdb3 -DDEBUG
DFLAGS = -fPIC $(PERLCXX) 
CXXFLAGS = $(DEBUG) $(WARNINGS) $(DFLAGS) -Iheaders/ -Isource/
ACXXFLAGS = $(DEBUG) $(WARNINGS) -Iheaders/
#CXXFLAGS = -Os -fomit-frame-pointer $(DFLAGS)
LDFLAGS = -Lblib -lperl++
LIBLDFLAGS := $(shell $(PERL) -MExtUtils::Embed -e ldopts)
LIBRARY_VAR=LD_LIBRARY_PATH

LIBNAME = libperl++.so
LIBTAPNAME = libtap++.so
LIB = blib/$(LIBNAME)
LIBTAP = blib/$(LIBTAPNAME)

HDRS := $(wildcard *.h)
PRESRCS := array.C call.C evaluate.C exporter.C glob.C hash.C handle.C helpers.C interpreter.C primitives.C reference.C regex.C regex_foreign.C scalar.C
SRCS := $(patsubst %,source/%,$(PRESRCS))
OBJS := $(patsubst %.C,blib/%.o,$(PRESRCS))

TEST_SRCS := $(wildcard t/*.C)
TEST_OBJS := $(patsubst %.C,%.t,$(TEST_SRCS))
TEST_GOALS = $(TEST_OBJS)

all: blib $(LIB) $(LIBTAP) blib/combined blib/game blib/Extend.so

source/ppport.h:
	perl -MDevel::PPPort -eDevel::PPPort::WriteFile\(\'$@\'\)

ppport: source/ppport.h

#$(LIB): $(OBJS)
#	ar -cr $(LIB) $(OBJS)
#	ranlib $(LIB)

blib:
	mkdir blib

headers:   

$(LIB): headers/config.h $(OBJS)
	gcc -shared -o $@ -Wl,-soname,$(LIBNAME) $(OBJS) $(LIBLDFLAGS)

$(LIBTAP): source/tap++.C headers/tap++.h
	gcc -fPIC -shared -o $@ -Wl,-soname,$(LIBTAPNAME) -Iheaders/ source/tap++.C

blib/%.o: source/%.C 
	$(CXX) $(CXXFLAGS) -c $< -o $@

t/%.t: t/%.C $(LIB) $(LIBTAP)
	$(CXX) $(ACXXFLAGS) -Lblib -lperl++ -ltap++ -o $@ $< 

source/evaluate.C: source/evaluate.C.PL
	perl $< > $@

headers/%.h: source/%.pre
	cpp $(PERLCXX) $< > $@

blib/combined: examples/combined.C $(LIB)
	$(CXX) -o $@ $(ACXXFLAGS) $< $(LDFLAGS)

blib/game: examples/game.C $(LIB)
	$(CXX) -o $@ $(ACXXFLAGS) $< $(LDFLAGS)

blib/Extend.so: examples/Extend.C $(LIB) headers/extend.h
	$(CXX) -o $@ -shared -fPIC $(ACXXFLAGS) $< $(LDFLAGS)

blib/interpreter.o: headers/extend.h

testbuild: $(TEST_GOALS)

test: $(TEST_GOALS)
	@echo run_tests.pl $(TEST_GOALS)
	@$(LIBRARY_VAR)=blib ./run_tests.pl $(TEST_GOALS)

prove: $(TEST_GOALS)
	@echo prove $(TEST_GOALS)
	@$(LIBRARY_VAR)=blib prove -e"sh -c" $(TEST_GOALS)

clean:
	-rm -r tap_tester examples/combined source/ppport.h source/evaluate.C headers/config.h headers/extend.h blib $(wildcard examples/*.o) $(wildcard t/*.o) $(wildcard t/*.t) 2>/dev/null

testclean:
	-rm t/*.t 2>/dev/null

again: clean all

love:
	@echo Not war?

lines:
	@wc -l source/*.C `ls headers/*.h | grep -v ppport.h` | sort -gr
linesh:
	@wc -l `ls headers/*.h | grep -v ppport.h` | sort -gr
linesC:
	@wc -l source/*.C | sort -gr

install: $(LIB)
	cp -a libperl++.so /usr/local/lib/

.PHONY: wordsC wordsh words lines linesh linesC todo install test prove testbuild ppport clean testclean all

words: 
	@make -s wordsC wordsh | sort -gr | column -t

wordsC:
	@(for i in source/*.C; do cpp -fpreprocessed $$i | sed 's/[_a-zA-Z0-9][_a-zA-Z0-9]*/x/g' | tr -d ' \012' | wc -c | tr "\n" " " ; echo $$i; done) | sort -gr | column -t;
wordsh:
	@(for i in `ls headers/*.h | grep -v ppport.h`; do cpp -fpreprocessed $$i 2>/dev/null | sed 's/[_a-zA-Z0-9][_a-zA-Z0-9]*/x/g' | tr -d ' \012' | wc -c | tr "\n" " " ; echo $$i; done) | sort -gr | column -t;

todo:
	@for i in FIX''ME XX''X TO''DO; do echo -n "$$i: "; $(ACK) $$i | wc -l; done;

apicount: blib/libperl++.so
	@echo -n "Number of entries: "
	@nm blib/libperl++.so -C --defined-only | egrep -i " [TW] perl::" | wc -l
