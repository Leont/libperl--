#include <perl++/perl++.h>
#include <tap++/tap++.h>

using namespace perl;
using namespace TAP;

void first() {
	ok(1, "void first();");
}

void second(int number) {
	ok(number, "void second(int)");
}

int third() {
	return 1;
}

int fourth(int arg) {
	return arg;
}

void fifth(Array ) {
	ok(1, "void fifth(Array)");
}

void sixth(Array args) {
	ok(args.length(), "void sixth(Array)");
}

void seventh(Array args) {
	ok(args[0], "void seventh(Array)");
}

const Scalar::Temp eigth(Array args) {
	return args[0];
}

int main() {
	plan(8);

	Interpreter universe;
	universe.add("first", first);
	universe.call("first");
	universe.add("second", &second);
	universe.call("second", 1);
	universe.add("third", &third);
	ok(universe.call("third"), "int third();");
	universe.add("fourth", &fourth);
	ok(universe.call("fourth", 1), "int fourth(int);");
	universe.add("fifth", &fifth);
	universe.call("fifth", 1);
	universe.add("sixth", &sixth);
	universe.call("sixth", 1);
	universe.add("seventh", &seventh);
	universe.call("seventh", 1);
	universe.add("eigth", &eigth);
	ok(universe.call("eigth", 1), "const Scalar::Temp eight(1);");

	return exit_status();
}
