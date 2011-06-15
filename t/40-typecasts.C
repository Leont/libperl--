#include <vector>
#include <perl++/perl++.h>
#include <tap++/tap++.h>

using namespace perl;
using namespace TAP;
using std::vector;

struct my_type {
	int value;
	my_type(int _value) : value(_value) {
	}
};

namespace perl {
	namespace typecast {
		template<> struct typemap<my_type> {
			static my_type cast_to(const Scalar::Base& value) {
				return my_type(value.int_value());
			}
			typedef boost::true_type from_type;
			static int cast_from(Interpreter&, const my_type& variable) {
				return variable.value;
			}
		};
	}
}

int main() {
	plan(6);

	Interpreter universe;

	Scalar scalar = universe.value_of(1);

	int foo = typecast_to<int>(scalar);
	is(foo, 1, "foo == 1");

	const char* foo2 = typecast_to<const char*>(scalar);
	is(foo2, std::string("1"), "foo2 == \"1\"");

	std::string foo3 = typecast_to<const char*>(scalar);
	is(foo3, std::string("1"), "foo3 == \"1\"");

	my_type bar = typecast_to<my_type>(scalar);
	is(bar.value, 1, "bar.value == 1");

	is(typecast_from(universe, 1), 1, "typecast_from(1) == 1");

	is(typecast_from(universe, bar), 1, "typecast_from(bar) == 1");

	Array baz = universe.eval_list("(1, 2, 3)");

	vector<my_type> buz = typecast_to<vector<my_type> >(baz);

	return exit_status();
}
