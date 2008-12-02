#include "perl++.h"
#include "tap++.h"

using namespace perl;
using namespace TAP;

int main(int argc, char** argv) {
	plan(8);
	Interpreter universe;
	String value = universe.value_of("test");

	ok(value, "value");
	is(value, "test", "value == \"test\"");
	isnt(value, "tset", "value != \"tset\"");
	is(value, std::string("test"), "value = std::string(\"test\")");
	is(value.length(), 4u, "length(value) == 4");

	note("value.replace(2, 2, value)");
	value.replace(2, 2, value);
	is(value, "tetest", "value = \"tetest\"");
	note("value.insert(2, \"st\")");
	value.insert(2, "st");
	is(value, "testtest", "value == \"testest\"");

	note("value = \"test\"");
	value = "test";
	is(value, "test", "value == \"test\"");

	return exit_status();
}
