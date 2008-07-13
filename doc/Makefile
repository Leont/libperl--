POD = pod2html
PERLPOD := $(shell perl -MExtUtils::Embed -e ccopts)
PODFLAGS = 

SRCS := $(wildcard *.pod)
OBJS := $(patsubst %.pod, %.html, $(SRCS))

all: $(OBJS)

%.html: %.pod
	$(POD) $(PODFLAGS) $< -outfile=$@

clean:
	-find ./ -name "*.html" | xargs rm

again: clean all

love:
	@echo Not war

lines:
	@wc -l *.pod | sort -gr
