#include "perl++.h"
#include <utility>
#include <algorithm>

using namespace perl;
using std::cout;
using std::endl;

void printer(const Raw_string& key, const Scalar::Base& value) {
	cout << key << " => " << value << endl;
}

void test(const Scalar::Base& val) {
	cout << "Value is '" << val << "'" << endl;
}

int test1(Array::Value& arg) {
	arg.each(test);
	return 42;
}

int test2(Argument_stack& stack) {
	Array arg = stack.get_arg();
//	cout << "Now calling tail" << endl;
//	stack.call(*sub_ref);
//	cout << "Called tail" << endl;
	return test1(arg);
//	return 42;
}

class mag {
	public:
//	void get_value(Scalar::Lvalue& val) {
//		cout << "Getting value '" << val << "'" << std::endl;
//	}
	void set_value(Scalar::Value& val) {
//		val = 1;
		cout << "Setting value to '" << val << "'" << endl;
	}
};

class tester {
	public:
	tester(int num = 0) : number(num) {
	}
	int number;
	void set(int new_value) {
		number = new_value;
	}
	void print() {
		cout << "a = " << number << endl;
	}
	~tester() {
		cout << "tester gets destroyed" << endl;
	}

	const std::pair<int, double> pair(std::pair<double, int> args) {
		return std::make_pair(args.second, args.first);
	}
};

struct member {
	int count;
};

int converter(int arg) {
	return arg * arg;
}

int main() {
	Interpreter universe;
	Package dbi = universe.use("DBI");
//	Interpreter multiverse(universe.clone());
//	universe.set_current();

	try {
		cout << "Should be empty '" << universe.scalar("non_existent") << "'" << endl;
		universe.scalar("non_existent") = "Not anymore";
		cout << "Should not be empty '" << universe.scalar("non_existent") << "'" << endl;

		Ref<Hash> foo = dbi.call("connect", "dbi:SQLite:dbname=dbfile", "", "");

		Array bar = foo.call_list("selectrow_array", "SELECT * FROM test WHERE name = ?", universe.undef(), "a");
//		Array bar = universe.list("a", "b");		
		bar[4] = "e";
		cout << "Array is " << bar.length() << " elements long" << endl;
		std::for_each(bar.begin(), bar.end(), test);
		Ref<Array> baz = bar.take_ref();

		cout << "bla is " << baz[1] << endl;
		Ref<Scalar> zab = bar[1].take_ref();
		String zaab = *zab;
		*zab = 4;
		cout << "bla is " << baz[1] << endl;

		Ref<Any> baaz = baz;
		baaz = zab;
		Array oof = *baz; 

		Array quz = universe.list("foo", 42);
		quz.push("test", "duck", 1);

		Ref<Code> rab = universe.eval("sub { print \"@_\n\"}");
		rab("Hello", "World!");
		rab(bar);
		bar.each(rab);

		Array singles = universe.list(1, 2, 3, 4);
		singles.map(converter).each(test);

		Hash map = universe.hash();
		map["test"] = "duck";
		map["foo" ] = "bar"; 
		map.each(printer);

//		Ref< Ref<Any> > doubleref = universe.eval("\\\\1");

		universe.add("test", test);
		universe.eval("test('test works')");

		universe.add("complex", test1);
		Ref<Code> complex = universe.eval("\\&complex");
		int test = complex("foo", bar, quz);
		cout << "complex('foo', bar, quz) : " << test << endl;

		mag set_magic;
		magical::writeonly(zaab, set_magic, &mag::set_value);
		cout << "zaab is " << zaab << endl;
		zaab = "220 Boe!";
		cout << "zaab is " << zaab << endl;

		int number(0);
		universe.add("foo", number);
		universe.eval("$foo = 42");
		cout << "foo is " << number << endl;

		Class<tester> classr = universe.add_class("Tester");
		classr.add(init<int>());
		classr.add("print", &tester::print);
		classr.add("set", &tester::set);
		classr.add("pair", &tester::pair);

		{
		Ref<Any> testr = universe.package("Tester").call("new", 1);
		testr.call("print");
		testr.call("set", 3);
		testr.call("print");
		}

		Class<member> classm = universe.add_class("Member");
		classm.add(init<>());
		classm.add("count", &member::count);

		Ref<Any> testm = universe.package("Member").call("new");
		cout << "testm.count = " << testm.call("count") << endl;
		testm.call("count", 1);
		cout << "testm.count = " << testm.call("count") << endl;

		String packed = universe.pack("Nni", 1001, 32768, -4096);
		Array unpacked = packed.unpack("Nni");
		test1(unpacked);

		Handle out = universe.open(">test.out");
		out.print("Foo\n");

		Regex regex = universe.regex("^\\w");
		if ( zaab.match(regex)) {
			cout << "It matches!" << endl;
		}
		zaab.substitute(universe.regex("(B\\w{2})"), "Muah $1");
		cout << "zaab is " << zaab << endl;

		cout << "End of story" << endl;
	} catch(Runtime_exception& ex) {
		cout << "Exception '" << ex.what() << "', thrown" << endl;
		std::terminate();
	}
	return 0;
}
