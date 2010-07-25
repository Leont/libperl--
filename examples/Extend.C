#include <perl++/perl++.h>
#include <perl++/extend.h>

using namespace perl;

static void hello(const char* world) {
	printf("Hello %s\n", world);
}

static void exporter(Package& extend) {
	extend.add("hello", hello);
}

EXPORTER(exporter);
