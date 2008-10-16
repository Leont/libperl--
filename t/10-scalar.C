#include "perl++.h"
#define WANT_TEST_EXTRAS
#include "tap++.h"

using namespace perl;
using namespace TAP;

int main(int argc, char** argv) {
	TEST_START(65);
	Interpreter universe;

	is_convertible<Scalar, int>("is_convertible<Scalar, int>()");
	is_convertible<Scalar, unsigned>("is_convertible<Scalar, unsigned>()");
	is_convertible<Scalar, long>("is_convertible<Scalar, long>()");
	is_convertible<Scalar, unsigned long>("is_convertible<Scalar, unsigned long>()");

	is_convertible<Scalar, double>("is_convertible<Scalar, double>()");
	is_convertible<Scalar, float>("is_convertible<Scalar, float>()");
	is_convertible<Scalar, long double>("is_convertible<Scalar, long float>()");

	TRY_DECL(Scalar value = universe.value_of(1), "Definition of value");
	diag("value = universe.value_of(1)");
	is(value, 1, "value == 1");
	is(value, 1u, "value == 1u");
	is(value, 1l, "value == 1l");
	is(value, "1", "value == \"1\"");
	is(value, 1.0, "value == 1.0");
	isnt(value, 2.0, "value != 2.0");
	is(value, 1.0f, "value == 1.0f");
	isnt(value, 2.0f, "value != 2.0f");

	is(static_cast<int>(value), 1, "(int)value == 1");
	ok(value > 0, "value > 0");
	ok(value < 2, "value < 2");
	not_ok(value != 1, "not: value != 1");
	not_ok(value != "1", "not: value != \"1\"");

	ok(value + 1, "value + 1");
	not_ok(value - 1 != 0, "not: value - 1");
	ok(value * 1, "value * 1");
	ok(value / 1, "value / 1");

	value += 2;
	diag("value += 2");

	is(value, 3, "value == 3");
	isnt(value, 4, "value != 4");

	value *= 3;
	diag("value *= 2");
	is(value, 9, "value == 9");

	value -= 1;
	diag("value -= 1");
	is(value, 8, "value == 8");

	value %= 5;
	diag("value %= 5");
	is(value, 3, "value == 3");

	value /= 3;
	diag("value /= 3");
	is(value, 1, "value == 1");

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

	is(++value, 2, "++value == 2");
	is(--value, 1, "--value == 1");
	TEST_END;
}
