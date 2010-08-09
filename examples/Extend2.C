#include <perl++/perl++.h>
#include <perl++/extend.h>

using namespace perl;

struct foo {
	foo(const std::string&);
	void bar(const char* world);
	std::string buz;
};

static void exporter(Class<foo>& extend) {
	extend.add(init<std::string>());
	extend.add("bar", &foo::bar);
	extend.add("buz", &foo::buz);
}

EXPORTER(exporter);
