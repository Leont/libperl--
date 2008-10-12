CXX = g++
ACK = ack-grep
PERL = perl
#WARNINGS = -Wall -Weffc++ -Wshadow -Wno-non-virtual-dtor
WARNINGS = -Wall -Wshadow
PERLCXX := $(shell $(PERL) -MExtUtils::Embed -e ccopts)
#PERLCXX=-D_REENTRANT -D_GNU_SOURCE -DTHREADS_HAVE_PIDS -DDEBIAN -fno-strict-aliasing -pipe -I/usr/local/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -I../../../Programs/perl/perl-5.10.0/
#PERLCXX=-D_REENTRANT -D_GNU_SOURCE -DTHREADS_HAVE_PIDS -DDEBIAN -fno-strict-aliasing -pipe -I/usr/local/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -I../../../Programs/perl/perl-current/
DEBUG = -ggdb3 -DDEBUG
DFLAGS = -fPIC $(PERLCXX) 
CXXFLAGS = $(DEBUG) $(WARNINGS) $(DFLAGS)
ACXXFLAGS = $(DEBUG) $(WARNINGS)
#CXXFLAGS = -Os -fomit-frame-pointer $(DFLAGS)
LDFLAGS = -L. -lperl++
LIBLDFLAGS := $(shell $(PERL) -MExtUtils::Embed -e ldopts)
PWD := $(shell pwd)
LIBRARY_VAR=LD_LIBRARY_PATH

LIB = libperl++.so

HDRS := $(wildcard *.h)
SRCS := array.C call.C exporter.C glob.C hash.C handle.C helpers.C interpreter.C magic.C parsed.C primitives.C reference.C regex.C scalar.C tap++.C
OBJS := $(patsubst %.C,%.o,$(SRCS))

TODEL := $(wildcard *.o) $(wildcard t/*.t)

TEST_SRCS := $(wildcard t/*.C)
TEST_OBJS := $(patsubst %.C,%.t,$(TEST_SRCS))
TEST_GOALS = $(TEST_OBJS)

all: $(LIB) example

ppport.h:
	perl -MDevel::PPPort -eDevel::PPPort::WriteFile

#$(LIB): $(OBJS)
#	ar -cr $(LIB) $(OBJS)
#	ranlib $(LIB)

$(LIB): definitions.h $(OBJS)
	gcc -shared -o $@ -Wl,-soname,$@ $(OBJS) $(LIBLDFLAGS)

%.o: %.C 
	$(CXX) $(CXXFLAGS) -c $< 

%.C: %.h

%.t: %.C
	$(CXX) $(ACXXFLAGS) -I $(PWD) -L $(PWD) -lperl++ -o $@ $< 

parsed.C: parsed.pl
	perl parsed.pl > parsed.C

definitions.h: definitions.pre
	cpp $(PERLCXX) $< > definitions.h

example: example.C
	$(CXX) -o $@ $(ACXXFLAGS) $< $(LDFLAGS)

test: $(LIB) $(TEST_GOALS)
	@echo Running unit tests
	@$(LIBRARY_VAR)=$(PWD) ./run_tests.pl $(TEST_GOALS)

#%.o: perl++.h

clean:
	-rm $(LIB) tap_tester example ppport.h parsed.C definitions.h $(TODEL) 2>/dev/null

testclean:
	-rm $(TEST_OBJS) 2>/dev/null

again: clean all

love:
	@echo Not war?

lines:
	@wc -l `ls *.[Ch] | grep -v ppport.h` | sort -gr
linesh:
	@wc -l `ls *.h | grep -v ppport.h` | sort -gr
linesC:
	@wc -l *.C | sort -gr

install: $(LIB)
	cp -a libperl++.so /usr/local/lib/

.PHONY: wordsC wordsh words lines linesh linesC todo install test

words: 
	@make -s wordsC wordsh | sort -gr | column -t

wordsC:
	@(for i in *.C; do cpp -fpreprocessed $$i | sed 's/[_a-zA-Z0-9][_a-zA-Z0-9]*/x/g' | tr -d ' \012' | wc -c | tr "\n" " " ; echo $$i; done) | sort -gr | column -t;
wordsh:
	@(for i in `ls *.h | grep -v ppport.h`; do cat $$i | sed 's/[_a-zA-Z0-9][_a-zA-Z0-9]*/x/g' | tr -d ' \012' | wc -c | tr "\n" " " ; echo $$i; done) | sort -gr | column -t;

todo:
	@for i in FIX''ME XX''X TO''DO; do echo -n "$$i: "; $(ACK) $$i | wc -l; done;

apicount: libperl++.so
	@echo -n "Number of entries: "
	@nm libperl++.so -C --defined-only | egrep -i " [TW] perl::" | wc -l
