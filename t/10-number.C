#include "perl++.h"
#include "tap++.h"
#include <values.h>
#include <cmath>

using namespace perl;

int main(int argc, char** argv) {
	TEST_START(16);
	Interpreter universe;
	TRY_DECL(Number value = universe.value_of(1.0), "value = value_of(1.0)");

	ok(value, "value");
	is_close(value, 1, "value == 1");
	is_close(value, 1u, "value == 1u");
	is_close(value, 1l, "value == 1l");

	ok(value > 0, "value > 0");
	ok(value < 2, "value < 2");
	not_ok(value != 1, "not: value != 1");

	ok(value + 1, "value + 1");
	ok(value * 1, "value * 1");
	ok(value / 1, "value / 1");

	diag("value -= 3");
	value -= 3;
	is_close(value, -2, "value == -2");
	is_remote(value, 1, "value != 1");
	std::printf("# value = %f\n", (double)value);

	diag("value *= -1");
	value *= -1;
	is_close(value, 2, "value == 2");
	std::printf("# value = %f\n", (double)value);

	diag("value = MAXDOUBLE");
	value = MAXDOUBLE;
	ok(value, "value");
	TEST_END;
}
