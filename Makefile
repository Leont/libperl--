CXX = g++
ACK = ack-grep
#WARNINGS = -Wall -Weffc++ -Wshadow -Wno-non-virtual-dtor
WARNINGS = -Wall -Wshadow
PERLCXX := $(shell perl -MExtUtils::Embed -e ccopts)
DEBUG = -ggdb3 -DDEBUG
DFLAGS = -fPIC $(PERLCXX) 
CXXFLAGS = $(DEBUG) $(WARNINGS) $(DFLAGS)
ACXXFLAGS = $(DEBUG) $(WARNINGS) -fPIC
#CXXFLAGS = -Os -fomit-frame-pointer $(DFLAGS)
LDFLAGS = -L. -lperl++
LIBLDFLAGS := $(shell perl -MExtUtils::Embed -e ldopts)

LIB = libperl++.so

HDRS := $(wildcard *.h)
SRCS := array.C call.C exporter.C glob.C hash.C helpers.C interpreter.C magic.C primitives.C reference.C scalar.C tap++.C
OBJS := $(patsubst %.C,%.o,$(SRCS))

TODEL := $(wildcard *.o)

all: $(LIB) example tap_tester

#$(LIB): $(OBJS)
#	ar -cr $(LIB) $(OBJS)
#	ranlib $(LIB)

$(LIB): $(OBJS)
	gcc -shared -o $@ -Wl,-soname,$@ $(OBJS) $(LIBLDFLAGS)

%.o: %.C 
	$(CXX) $(CXXFLAGS) -c $< 

%.C: %.h
	

tap_tester.o: tap_tester.C tap++.h
	$(CXX) $(ACXXFLAGS) -c $<

example.o: example.C
	$(CXX) $(ACXXFLAGS) -c $<

tap_tester: tap_tester.o
	$(CXX) -o $@ $< $(LDFLAGS)

example: example.o
	$(CXX) -o $@ $< $(LDFLAGS)

#%.o: perl++.h

clean:
	-rm $(LIB) tap_tester example $(TODEL) 2>/dev/null

again: clean all

love:
	@echo Not war?

lines:
	@wc -l *.[Ch] | sort -gr
linesh:
	@wc -l *.h | sort -gr
linesC:
	@wc -l *.C | sort -gr

install: $(LIB)
	cp -a libperl++.so /usr/local/lib/

.PHONY: wordsC wordsh words lines linesh linesC todo install

words: 
	@make -s wordsC wordsh | sort -gr | column -t

wordsC:
	@(for i in *.C; do cpp -fpreprocessed $$i | sed 's/[_a-zA-Z0-9][_a-zA-Z0-9]*/x/g' | tr -d ' \012' | wc -c | tr "\n" " " ; echo $$i; done) | sort -gr | column -t;
wordsh:
	@(for i in *.h; do cat $$i | sed 's/[_a-zA-Z0-9][_a-zA-Z0-9]*/x/g' | tr -d ' \012' | wc -c | tr "\n" " " ; echo $$i; done) | sort -gr | column -t;

todo:
	@for i in FIX''ME XX''X TO''DO; do echo -n "$$i: "; $(ACK) $$i | wc -l; done;
