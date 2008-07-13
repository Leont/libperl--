#include "perl++.h"
#include "tap++.h"

using namespace perl;
using std::cout;
using std::endl;

int main() {
	SANE(Interpreter universe, "Can create interpreter");

	expect(2);
	SANE(Package dbi = universe.use("DBI") , "Can use...");

	SANE(universe.eval("1"), "eval '1'");
	return 0;
}
