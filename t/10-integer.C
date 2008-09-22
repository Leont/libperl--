#include "perl++.h"
#include "tap++.h"

using namespace perl;

int main(int argc, char** argv) {
	TEST_START(20);
	Interpreter universe;
	TRY_DECL(Integer value = universe.value_of(1), "value = value_of(1)");

	ok(value, "value");
	is(value, 1, "value == 1");
	is(value, 1u, "value == 1u");
	is(value, 1l, "value == 1l");

	ok(value > 0, "value > 0");
	ok(value < 2, "value < 2");
	not_ok(value != 1, "not: value != 1");

	ok(value + 1, "value + 1");
	not_ok(value - 1, "not: value - 1");
	ok(value * 1, "value * 1");
	ok(value / 1, "value / 1");

	diag("++value");
	++value;
	ok(value, "value");
	is(value, 2, "value == 2");
	not_ok(value != 2, "not: value != 2");

	diag("--value");
	--value;
	is(value, 1, "value == 1");

	diag("value++");
	value++;
	is(value, 2, "value == 2");

	diag("value -= 3");
	value -= 3;
	is(value, -1, "value == -1");

	diag("value *= -1");
	value *= -1;
	is(value, 1, "value == 1");

	diag("value = LONG_MAX");
	value = LONG_MAX;
	ok(value, "value");
	TEST_END;
}
