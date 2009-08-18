#include "perl++.h"
#include "tap++.h"
#include <climits>

using namespace perl;
using namespace TAP;

struct tester {
	void first() {
		ok(1, "$object->first");
	}

	void second(int number) {
		ok(number, "$object->second(1)");
	}

	int third() {
		return 1;
	}

	int fourth(int arg) {
		return arg;
	}

	void fifth(Array) {
		ok(1, "$object->fifth(Array)");
	}

	void sixth(Array args) {
		ok(args.length(), "$object->sixth(Array)");
	}

	void seventh(Array args) {
		ok(args[0], "$object->seventh(Array)");
	}

	const Scalar::Temp eigth(Array args) {
		return args[0];
	}
};

int main() {
	plan(9);

	Interpreter universe;
	Class<tester> testerc = universe.add_class("Tester");

	testerc.add(init<>());
	Scalar object = universe.package("Tester").call("new");
	ok(object, "$object = Tester->new");

	testerc.add("first", &tester::first);
	object.call("first");

	testerc.add("second", &tester::second);
	object.call("second", 1);

	testerc.add("third", &tester::third);
	ok(object.call("third"), "$object->third");

	testerc.add("fourth", &tester::fourth);
	ok(object.call("fourth", 1), "$object->fourth(1)");

	testerc.add("fifth", &tester::fifth);
	object.call("fifth");

	testerc.add("sixth", &tester::sixth);
	object.call("sixth", 1);

	testerc.add("seventh", &tester::seventh);
	object.call("seventh", 1);

	testerc.add("eigth", &tester::eigth);
	ok(object.call("eigth", 1), "$object->eight(1)");

	return exit_status();
}
