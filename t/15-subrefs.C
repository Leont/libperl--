#include "perl++.h"
#define WANT_TEST_EXTRAS
#include "tap++.h"

using namespace perl;
using namespace TAP;

int main(int argc, char** argv) {
	TEST_START(12);
	Interpreter universe;

	universe.eval(
		"sub first { return 1 }\n"
		"sub second { return scalar @_ }\n"
		"sub third { return $_[0] }\n"
	);
	Ref<Code> first = universe.eval("\\&first");
	ok(first(), "first()");

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

	// TODO: list context

	TEST_END;
}
