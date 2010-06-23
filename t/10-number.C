#include <perl++/perl++.h>
#include <tap++/tap++.h>
#include <values.h>
#include <cmath>

using namespace perl;
using namespace TAP;

int main() {
	plan(17);
	Interpreter universe;
	is_convertible<Number, double>("is_convertible<Number, double>()");
	is_convertible<Number, float>("is_convertible<Number, float>()");
	is_convertible<Number, long double>("is_convertible<Number, long double>()");
	Number value = universe.value_of(1.0);

	ok(value, "value");
	is(value, 1, "value == 1");
	is(value, 1u, "value == 1u");
	is(value, 1l, "value == 1l");

	ok(value > 0, "value > 0");
	ok(value < 2, "value < 2");
	ok(!(value != 1), "not: value != 1");

	ok(value + 1, "value + 1");
	ok(value * 1, "value * 1");
	ok(value / 1, "value / 1");

	note("value -= 3");
	value -= 3;
	is(value, -2, "value == -2");
	isnt(value, 1, "value != 1");

	note("value *= -1");
	value *= -1;
	is(value, 2, "value == 2");

	note("value = MAXDOUBLE");
	value = MAXDOUBLE;
	ok(value, "value");
	return exit_status();
}
