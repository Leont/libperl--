#include "perl++.h"
#include "tap++.h"
#include <climits>

using namespace perl;
using namespace TAP;

int main() {
	plan(3);

	Interpreter universe;
	String packed = universe.pack("Nni", 1001, 32768, -4096);
	Array unpacked = packed.unpack("Nni");

	is(unpacked[0], 1001, "unpacked[0] == 1001");
	is(unpacked[1], 32768, "unpacked[0] == 32768");
	is(unpacked[2], -4096, "unpacked[0] == -4096");
	// I need a decent substr for more complicated tests.

	return exit_status();
}
