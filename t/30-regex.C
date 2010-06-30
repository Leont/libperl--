#include <perl++/perl++.h>
#define WANT_TEST_EXTRAS
#include <tap++/tap++.h>
#include <climits>

using namespace perl;
using namespace TAP;

int main() {
	TEST_START(9);
	Interpreter universe;
	Regex first = universe.regex("a*b");
	ok(first.match(universe.value_of("aab")), "\"aab\" =~ /a*b/;");
	not_ok(first.match("aaa"), "\"aaa\" !~ /a*b/;");

	Array matches = universe.regex("a+").match("abaabaaabaaba", "g");

	is(matches, 5u, "Match returned elements");

	is(matches[1], "aa");

	String victim = universe.value_of("aab");

	ok(victim.match(first), "Victim matches");

	ok(victim.substitute(first, "saab"), "Victim substitutes");

	is(victim, "saab", "Victim now is saab");

	ok(victim.substitute(universe.regex("(a*b)"), "foo$1bar"), "Victim substitutes complexly");

	is(victim, "sfooaabbar", "Complex substitution works");

	TEST_END;
}
