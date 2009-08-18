#include "perl++.h"
#include "tap++.h"
#include <string>

using namespace perl;
using namespace TAP;

int main() {
	plan(8);

	Interpreter universe;

	int number = 1;
	universe.add("number", number);
	is(universe.scalar("number"), 1, "$number == 1");
	number++;
	is(universe.scalar("number"), 2, "$number == 2");
	int newval = universe.eval("$number++");
	is(newval, 2, "newval == 2");
	is(number, 3, "number == 3");

	std::string text = "";

	universe.add("string", text);
	is(universe.scalar("string"), "", "string == \"\"");
	text = "foo";
	is(universe.scalar("string"), "foo", "string == \"foo\"");
	std::string nextval = universe.eval("$string = 'bar'");
	is(nextval, "bar", "nextval == \"bar\"");
	is(text,    "bar", "nextval == \"bar\"");

	return exit_status();
}
