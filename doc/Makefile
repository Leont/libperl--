POD = pod2html
PERLPOD := $(shell perl -MExtUtils::Embed -e ccopts)
PODFLAGS = 

SRCS := $(wildcard *.pod)
OBJS := $(patsubst %.pod, %.html, $(SRCS))

all: objects
	
objects: $(OBJS)

%.html: %.pod
	$(POD) $(PODFLAGS) -infile=$< -outfile=$@

clean:
	find ./ -name "*.html" -delete
	find ./ -name "pod2htm*.tmp" -delete

again: clean all

love:
	@echo Not war.

lines:
	@wc -l *.pod | sort -gr

.PHONY: all objects clean ahain love lines
