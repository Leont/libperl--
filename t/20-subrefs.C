#include "perl++.h"
#define WANT_TEST_EXTRAS
#include "tap++.h"

using namespace perl;
using namespace TAP;

int main() {
	TEST_START(19);
	Interpreter universe;

	universe.eval(
		"sub first { return 1 }\n"
		"sub second { return scalar @_ }\n"
		"sub third { return $_[0] }\n"
		"sub fourth { return ++$_[0] }\n"
		"sub fifth { @_ }\n"
	);
	Ref<Code> first = universe.eval("\\&first");
	ok(first(), "first()");
	is(first(), 1, "first() == 1");

	Ref<Code> second = universe.eval("\\&second");
	ok(second(1), "second(1)");
	ok(second("foo"), "second(\"foo\")");

	is(second(1), 1, "second(1) == 1");
	is(second(1, 1), 2, "second(1, 1) == 2");

	Array more = universe.list(1, 2, 3);
	is(second(more), 3, "second(more) == 3");
	is(second(1, 1, more), 5, "second(1, 1, more) == 5");

	Ref<Code> third = universe.eval("\\&third");
	ok(third(1), "third(1)");
	ok(third("foo"), "third(\"foo\")");

	is(third(1), 1, "third(1) == 1");
	is(third ("foo"), "foo", "third(\"foo\") == \"foo\"");
	is(third(more), 1, "third(more) == 1");

	Ref<Code> fourth = universe.eval("\\&fourth");
	Integer value = universe.value_of(1);
	note("value = 1");

	is(fourth(value), 2, "fourth(value) == 2");
	is(value, 2, "value == 2");


	is(second.list(1)[0], 1, "second(1) == 1");
	is(second.list(1, 1)[0], 2, "second(1, 1) == 2");

	Ref<Code> fifth = universe.eval("\\&fifth");
	is(fifth.list(1, 2, 3).length(), 3u, "fifth(1, 2, 3).length() == 3");
	is(fifth.list(1, 2, 3)[2], 3, "list[2] == 3");

	TEST_END;
}
