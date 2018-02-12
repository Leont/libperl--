#include <perl++/perl++.h>
#include <tap++/tap++.h>
#include <climits>

using namespace perl;
using namespace TAP;

int main() {
	plan(19);
	Interpreter universe;
	Integer value = universe.value_of(1);

	ok(value, "value");
	is(value, 1, "value == 1");
	is(value, 1u, "value == 1u");
	is(value, 1l, "value == 1l");

	ok(value > 0, "value > 0");
	ok(value < 2, "value < 2");
	not_ok(value != 1, "not: value != 1");

	ok(value + 1, "value + 1");
	not_ok(value - 1, "not: value - 1");
	is(value * 1, 1, "value * 1");
	ok(value / 1, "value / 1");

	note("++value");
	++value;
	ok(value, "value");
	is(value, 2, "value == 2");
	not_ok(value != 2, "not: value != 2");

	note("--value");
	--value;
	is(value, 1, "value == 1");

	note("value++");
	value++;
	is(value, 2, "value == 2");

	note("value -= 3");
	value -= 3;
	is(value, -1, "value == -1");

	note("value *= -1");
	value *= -1;
	is(value, 1, "value == 1");

	note("value = LONG_MAX");
	value = LONG_MAX;
	ok(value, "value");
	return exit_status();
}
