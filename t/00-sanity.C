#include "perl++.h"
#include "tap++.h"

using namespace perl;

int main(int argc, char** argv) {
	TEST_START(18);

	FAIL(throw Runtime_exception("Runtime exception"), "Should throw a Runtime exception");
	FAIL(assertion<Runtime_exception>(false, "Runtime exception"), "Should throw a Runtime exception too");
	TRY(assertion<Runtime_exception>(true, "Runtime exception"), "Shouldn't throw a Runtime exception");

	Raw_string test = "Test";
	is(test, "Test", Raw_string("a Raw_string equals an equal strings"));
	is(test, "Test", "a Raw_string equals an equal const char*");
	//TODO: more tests on Raw_string

	TRY_DECL(Interpreter universe, "instantiating interpreter");
	diag("so far so good...");

	ok(universe.value_of(1), "value_of(1)");
	not_ok(universe.value_of(0), "value_of(0)");
	is(universe.value_of(1), 1, "value_of(1) is 1");
	isnt(universe.value_of(1), 2, "value_of(1) isn't 2");
	ok(universe.eval("1"), "eval 1");

	ok(universe.value_of("Test"), "value_of(\"Test\")");
	not_ok(universe.value_of(""), "value_of(\"\")");
//	skip("value_of(\"\")"); //How should this behave?
	is(universe.value_of("Test"), "Test", "value_of(\"Test\") is \"Test\"");

	not_ok(universe.scalar("_").defined(), "$_ is not defined");
	ok(universe.scalar("_") = "Anything", "$_ is assigned to");
	ok(universe.scalar("_").defined(), "$_ is defined");
	TRY_DECL(Package dbi = universe.use("DBI"), "use DBI");

	TEST_END;
}
