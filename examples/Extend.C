#include "perl++.h"
#include "extend.h"

using namespace perl;

static void world() {
	printf("Hello World!\n");
}

static void exporter(Package& interp) {
	interp.add("hello", world);
}

EXPORTER(exporter);
