#include "perl++.h"
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

		Array bar = foo.call_array("selectrow_array", "SELECT * FROM test WHERE name = ?", universe.undef(), "a");
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
		Array oof = *baz; 

		Array quz = universe.list("foo", 42);
		quz.push("test", "duck", 1);

		Ref<Code> rab = universe.eval("sub { print \"@_\n\"}");
		rab("Hello", "World!");
		rab(bar);

		Array singles = universe.list(1, 2, 3, 4);
		singles.map(converter).each(test);

		Hash map = universe.hash();
		map["test"] = "duck";
		map["foo" ] = "bar"; 
		map.each(printer);

//		ref< ref<any> > bugz = universe.eval("\\\\1");


		universe.export_sub("test", test);
		universe.eval("test('test works')");

		universe.export_flat("complex", test1);
		Ref<Code> complex = universe.eval("\\&complex");
		int test = complex("foo", bar, quz);
		cout << "complex('foo', bar, quz) : " << test << endl;

		mag magic;
		Magic::writeonly(zaab, magic, &mag::set_value);
		cout << "zaab is " << zaab << endl;
		zaab = "220 Boe!";
		cout << "zaab is " << zaab << endl;

		Class<tester> classr = universe.add_class("Tester");
		classr.add(init<int>());
		classr.add("print", &tester::print);
		classr.add("set", &tester::set);

		{
		Ref<Any> testr = universe.get_package("Tester").call("new", 1);
		testr.call("print");
		testr.call("set", 3);
		testr.call("print");
		}
		String packed = universe.pack("Nni", 1001, 32768, -4096);
		Array unpacked = packed.unpack("Nni");
		test1(unpacked);
		packed = universe.list(1001, 32768, -4096).pack("Nni");
		unpacked = packed.unpack("Nni");
		test1(unpacked);
		cout << "End of story" << endl;
	} catch(Runtime_exception& ex) {
		cout << "Exception '" << ex.what() << "', thrown" << endl;
		std::terminate();
	}
	return 0;
}
