#include "perl++.h"
#include "tap++.h"

using namespace perl;
using namespace TAP;
using std::cout;
using std::endl;

int main() {
	TEST_START;
	plan(3);
	TRY_DECL(Interpreter universe, "Create interpreter");

	TRY_DECL(Package dbi = universe.use("DBI"), "use DBI");

	TRY(universe.eval("1"), "eval '1'");
	TEST_END;
	return 0;
}
