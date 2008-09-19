#include "perl++.h"
#include "tap++.h"

using namespace perl;

int main(int argc, char** argv) {
	TEST_START(10);

	TRY_DECL(Interpreter universe, "instantiating interpreter");
	diag("so far so good...");

	ok(universe.value_of(1), "value_of(1)");
	not_ok(universe.value_of(0), "value_of(0)");
	is(universe.value_of(1), 1, "value_of(1) is 1");
	isnt(universe.value_of(1), 2, "value_of(1) isn't 2");
	ok(universe.eval("1"), "eval 1");

	ok(universe.value_of("Test"), "value_of(\"Test\")");
//	not_ok(universe.value_of(""), "value_of(\"\")");
	skip("value_of(\"\")");
	is(universe.value_of("Test"), "Test", "value_of(\"Test\") is \"Test\"");

	TRY_DECL(Package dbi = universe.use("DBI"), "use DBI");

	TEST_END;
}
