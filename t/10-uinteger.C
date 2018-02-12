#include <perl++/perl++.h>
#include <tap++/tap++.h>

using namespace perl;
using namespace TAP;

int main() {
	plan(15);
	Interpreter universe;
	Uinteger value = universe.value_of(1u);

	ok(value, "value");
	is(value, 1u, "value == 1u");

	ok(value > 0u, "value > 0");
	ok(value < 2u, "value < 2");
	not_ok(value != 1, "not: value != 1");

	ok(value + 1, "value + 1");
	not_ok(value - 1, "not: value - 1");
	is(value * 1u, 1u, "value * 1");
	ok(value / 1, "value / 1");

	note("++value");
	++value;
	ok(value, "value");
	is(value, 2u, "value == 2");
	not_ok(value != 2, "not: value != 2");

	note("--value");
	--value;
	is(value, 1u, "value == 1");

	note("value++");
	value++;
	is(value, 2u, "value == 2");

	note("value -= 2");
	value -= 2u;
	is(value, 0u, "value == 0");

	return exit_status();
}
