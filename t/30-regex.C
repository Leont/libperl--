#include "perl++.h"
#define WANT_TEST_EXTRAS
#include "tap++.h"
#include <climits>

using namespace perl;
using namespace TAP;

int main() {
	TEST_START(2);
	Interpreter universe;
	Regex first = universe.regex("a*b");
	ok(first.match(universe.value_of("aab")), "\"aab\" =~ /a*b/;");
	not_ok(first.match("aaa"), "\"aaa\" !~ /a*b/;");
	TEST_END;
}
