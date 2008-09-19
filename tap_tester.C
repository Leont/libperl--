#include "perl++.h"
#include "tap++.h"

using namespace perl;

int main() {
	TEST_START(5);
	TRY_DECL(Interpreter universe, "Create interpreter");

	TRY_DECL(Package dbi = universe.use("DBI"), "use DBI");
	
	FAIL(throw Runtime_exception("foo"), "Should throw");
	skip("Nothing");
	TRY(universe.eval("1"), "eval '1'");
	TEST_END;
	return 0;
}
