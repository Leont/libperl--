#include "perl++.h"
#include "tap++.h"

using namespace perl;
using namespace TAP;
using std::cout;
using std::endl;

int main() {
	TEST_START;
	plan(3);
	TRY_MESSAGE("Create interpreter");
	Interpreter universe;

	TRY_MESSAGE("use DBI");
	Package dbi = universe.use("DBI");

	TRY(universe.eval("1"), "eval '1'");
	TEST_END;
	return 0;
}
