#include "perl++.h"
#include "tap++.h"

using namespace perl;

int main(int argc, char** argv) {
	TEST_START(45);
	Interpreter universe;
	TRY_DECL(Scalar value = universe.value_of(1), "Definition of value");
	diag("value = universe.value_of(1)");
	is(value, 1, "value == 1");
	is(value, 1u, "value == 1u");
	is(value, 1l, "value == 1l");
	is(value, "1", "value == \"1\"");

	ok(value > 0, "value > 0");
	ok(value < 2, "value < 2");
	not_ok(value != 1, "not: value != 1");
	not_ok(value != "1", "not: value != \"1\"");

	ok(value + 1, "value + 1");
	not_ok(value - 1 != 0, "not: value - 1");
	ok(value * 1, "value * 1");
	ok(value / 1, "value / 1");

	TRY_DECL(int foo = value, "int foo = value");
	ok(foo == 1, "foo == 1");
	
	value = 1;
	diag("value = 1");
	is(value, 1, "value == 1");
	is(value, 1u, "value == 1u");
	is(value, 1l, "value == 1l");
	is(value, "1", "value == \"1\"");

	ok(value + 1, "value + 1");
	not_ok(value - 1, "not: value - 1");
	ok(value * 1, "value * 1");
	ok(value / 1, "value / 1");

	value = 1u;
	diag("value = 1u");
	is(value, 1, "value == 1");
	is(value, 1u, "value == 1u");
	is(value, 1l, "value == 1l");
	is(value, "1", "value == \"1\"");

	ok(value + 1, "value + 1");
	not_ok(value - 1, "not: value - 1");
	ok(value * 1, "value * 1");
	ok(value / 1, "value / 1");

	ok(value > 0, "value > 0");
	ok(value < 2, "value < 2");
	not_ok(value != 1, "not: value != 1");

	value = "1";
	diag("value = \"1\"");
	is(value, 1, "value == 1");
	is(value, 1u, "value == 1u");
	is(value, 1l, "value == 1l");
	is(value, "1", "value == \"1\"");

	ok(value > 0, "value > 0");
	ok(value < 2, "value < 2");
	not_ok(value != 1, "not: value != 1");

	ok(value + 1, "value + 1");
	not_ok(value - 1, "not: value - 1");
	ok(value * 1, "value * 1");
	ok(value / 1, "value / 1");

	TEST_END;
}
