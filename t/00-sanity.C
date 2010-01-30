#include "perl++.h"
#define WANT_TEST_EXTRAS
#include "tap++.h"

using namespace perl;
using namespace TAP;

int main() {
	TEST_START(39);

	ok(true, "True is ok");
	not_ok(false, "False is not ok");
	is(1, 1, "1 == 1");
	isnt(1, 2, "1 == 2");
	is(1.0, 1.0, "1.0 == 1.0");
	isnt(1.0, 2.0, "1.0 != 2.0");

	FAIL(throw Runtime_exception("Runtime exception"), "Should throw a Runtime exception");
	FAIL(assertion<Runtime_exception>(false, "Runtime exception"), "Should throw a Runtime exception too");
	TRY(assertion<Runtime_exception>(true, "Runtime exception"), "Shouldn't throw a Runtime exception");
	is_convertible<int, long>("int can be converted into a long");
	is_inconvertible<int, int(*)()>("int can't be converted into an function pointer");

	BLOCK_START(1);
		skip("Should skip");
		is(0, 1, "0 == 1");
	BLOCK_END;

	Raw_string test = "Test";
	is(test, Raw_string("Test"), "a Raw_string equals an equal Raw_string");
	is(test, "Test", "a Raw_string equals an equal const char*");
	//TODO: more tests on Raw_string

	TRY_DECL(Interpreter universe, "instantiating interpreter");
	note("so far so good...");

	ok(universe.value_of(1), "value_of(1)");
	not_ok(universe.value_of(0), "value_of(0)");
	is(universe.value_of(1), 1, "value_of(1) is 1");
	isnt(universe.value_of(1), 2, "value_of(1) isn't 2");
	is(universe.value_of(2), 2, "value_of(2) is 2");
	isnt(universe.value_of(2), 1, "value_of(2) isn't 1");
	ok(universe.eval("1"), "eval 1");

	ok(universe.value_of("Test"), "value_of(\"Test\")");
	not_ok(universe.value_of(""), "value_of(\"\")");
	is(universe.value_of("Test"), "Test", "value_of(\"Test\") is \"Test\"");

	not_ok(universe.scalar("_").defined(), "$_ is not defined");
	ok(universe.scalar("_") = "Anything", "$_ is assigned to");
	ok(universe.scalar("_").defined(), "$_ is defined");

	is_convertible<Scalar::Temp, Scalar>("Scalar::Temp can be converted into a Scalar");
	is_convertible<Integer::Temp, Scalar>("Integer::Temp can be converted into a Scalar");
	is_convertible<Integer, Scalar>("Integer can be converted into a Scalar");
	is_inconvertible<Integer, String>("Integer can't be converted into a String");

	is_convertible<Scalar::Temp, Integer>("Can convert a Scalar::Temp into a Integer");
	is_convertible<Scalar::Temp, Uinteger>("Can convert a Scalar::Temp into a UInteger");
	is_convertible<Scalar::Temp, Number>("Can convert a Scalar::Temp into a Number");
	is_convertible<Scalar::Temp, String>("Can convert a Scalar::Temp into a String");
	is_convertible<Scalar::Temp, Ref<Any> >("Can convert a Scalar::Temp into a Ref<Any>");

	TRY_DECL(Scalar value = universe.value_of(1), "value = value_of(1)");

	TRY_DECL(Package dbi = universe.use("DBI"), "use DBI");

	TEST_END;
}
