#include "perl++.h"
#define WANT_TEST_EXTRAS
#include "tap++.h"

using namespace perl;
using namespace TAP;

int main() {
	TEST_START(20);
	Interpreter universe;

	universe.eval(
		"package Object;\n"
		"sub new { bless {}, $_[0] }\n"
		"sub first { return 1 }\n"
		"sub second { return scalar @_ }\n"
		"sub third { return $_[1] }\n"
		"sub fourth { return ++$_[1] }\n"
		"sub fifth { @_ }\n"
	);
	Package package = universe.package("Object");
	Ref<Any> object = package.call("new");
	ok(1, "Constructed the object without errors!");
	diag("Here!");
	ok(object.call("first", "object.first()"));
	diag("There!");
	is(object.call("first"), 1, "object.first() == 1");

	ok(object.call("second", 1, "object.second(1)"));
	ok(object.call("second", "foo", "object.second(\"foo\")"));

	is(object.call("second", 1), 2, "object.second(1) == 2");
	is(object.call("second", 1, 1), 3, "object.second(1, 1) == 3");

	Array more = universe.list(1, 2, 3);
	is(object.call("second", more), 4, "object.second(more) == 4");
	is(object.call("second", 1, 1, more), 6, "object.second(1, 1, more) == 6");

	ok(object.call("third", 1), "object.third(1)");
	ok(object.call("third", "foo"), "object.third(\"foo\")");

	is(object.call("third", 1), 1, "object.third(1) == 1");
	is(object.call("third", "foo"), "foo", "object.third(\"foo\") == \"foo\"");
	is(object.call("third", more), 1, "object.third(more) == 1");

	Integer value = universe.value_of(1);
	note("value = 1");

	is(object.call("fourth", value), 2, "object.fourth(value) == 2");
	is(value, 2, "value == 2");

	is(object.call_list("second", 1)[0], 2, "object.second(1) == 2");
	is(object.call_list("second", 1, 1)[0], 3, "object.second(1, 1) == 3");

	is(object.call_list("fifth", 1, 2, 3).length(), 4u, "object.fifth(1, 2, 3).length() == 3");
	is(object.call_list("fifth", 1, 2, 3)[3], 3, "list[3] == 3");
	TEST_END;
}
