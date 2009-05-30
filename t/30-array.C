#include "perl++.h"
#include "tap++.h"
#include "lambda.h"
#include <algorithm>
#include <limits>

using std::numeric_limits;

using namespace perl;
using namespace TAP;

#define UV_MAX numeric_limits<UV>::max()
#define IV_MAX numeric_limits<IV>::max()
#define NV_MAX numeric_limits<NV>::max()

int main() {
	plan(65);
	Interpreter universe;
	Array array = universe.list();
	is(array.length(), 0u, "array.length() == 0");

	array.push(1);
	note("array.push(1)");
	is(array[0], 1, "array[0] == 1");
	is(array[0], "1", "array[0] == \"1\"");

	++array[0];
	note("++array[0]");
	is(array[0], 2, "array[0] == 1");
	is(array[0], "2", "array[0] == \"1\"");

	note("array.push(UV_MAX)");
	array.push(UV_MAX);
	is(array[1], UV_MAX, "array[1] == UV_MAX");
	ok(array[1] > static_cast<UV>(IV_MAX), "array[1] > (unsigned)IV_MAX");
	is(array[1], -1, "array[1] == -1");

	array.push(NV_MAX);
	note("array.push(NV_MAX)");
	is(array[2], NV_MAX, "array[2] == NV_MAX");

	array.push("test");
	note("array.push(\"test\")");
	is(array[3], "test", "array[3] == \"test\"");

	array.push(Raw_string("test"));
	note("array.push(Raw_string(\"test\"))");
	is(array[4], "test", "array[4] == \"test\"");

	array.push(universe.value_of(1));
	note("array.push(universe.value_of(1))");
	is(array[5], 1, "array[4] == 1");

	array.clear();
	note("array.clear()");
	is(array.length(), 0u, "array.length() == 0");
	array.push(1, UV_MAX, NV_MAX, "test", universe.undef());
	note("array.push(1, UV_MAX, NV_MAX, \"test\")");

	is(array[0], 1, "array[0] == 1");
	is(array[1], UV_MAX, "array[1] == UV_MAX");
	is(array[2], NV_MAX, "array[2] == NV_MAX");
	is(array[3], "test", "array[3] == \"test\"");
	ok(array.exists(4), "exists array[4]");
	ok(!array[4].defined(), "not: defined array[4]");

	array.push(array);
	note("array.push(array)");
	is(array[5], 1, "array[5] == 1");
	is(array[6], UV_MAX, "array[6] == UV_MAX");
	is(array[7], NV_MAX, "array[7] == NV_MAX");
	is(array[8], "test", "array[8] == \"test\"");
	ok(array.exists(9), "exists array[9]");
	ok(!array[9].defined(), "not: defined array[9]");

	array.clear();
	array.unshift("test");
	array.unshift(NV_MAX);
	array.unshift(UV_MAX);
	array.unshift(1);
	note("array.unshift(1, UV_MAX, NV_MAX, \"test\")");

	is(array[0], 1, "array[0] == 1");
	is(array[1], UV_MAX, "array[1] == UV_MAX");
	is(array[2], NV_MAX, "array[2] == NV_MAX");
	is(array[3], "test", "array[3] == \"test\"");

	ok(array.exists(3), "exists array[3]");
	String string = array.remove(3);
	note("string = array.remove(3)");
	ok(string.length(), "string.length()");
	ok(!array.exists(3), "not: exists array[3]");
	is(array.length(), 3u, "array.length() == 3");

	array.length() = 2;
	note("array.length() = 2");
	ok(!array.exists(2), "not: exists array[2]");
	is(array.length(), 2u, "array.length() == 2");
	array.extend(4);
	note("array.extend(4)");
	is(array.length(), 2u, "array.length == 2");

	Array numbers = universe.list(1, 2, 3, 4);
	Array squares = numbers.map(ll_static_cast<IV>(_1) * _1);
	for (int i = 0; i < 4; ++i) {
		std::string num = boost::lexical_cast<std::string>(i);
		is(squares[i], (IV)(numbers[i]) * numbers[i], std::string("squares[") + num + "] is numbers[" + num + "] ** 2");
	}

	Array big = squares.grep(_1 > 8);
	note("Array big = squares.grep(_1 > 8)");
	is(big.length(), 2u, "big.length == 2");
	is(big[0], 9, "big[0] == 9");

	Array doubles = numbers.map(_1 * 2);
	is(doubles[3], 8, "numbers.map(_1 * 2)[3] == 8");

	IV sum = squares.reduce(_1 + _2, 0);
	note("sum = squares.reduce(_1 + _2, 0u)");
	is(sum, 30, "sum == 30");

	big.each(_1 *= 2);
	note("big.each(_1 *= 2)");
	is(big[0], 18, "big[0] == 18");

	const Array forties = universe.list(46, 47, 48, 49);
	ll_is(bind<int>(&TAP::encountered), _1, "encountered == 46")(make_const(45));
	std::for_each(forties.begin(), forties.end(), ll_is(bind(&TAP::encountered), _1, "encountered == 4x"));

	ok(forties.any(_1 == 48), "forties.any(_1 == 48)");
	ok(forties.all(_1 > 45), "forties.all(_1 > 45)");
	ok(forties.none(_1 == 30), "forties.none(_1 == 30)");
	ok(forties.notall(_1 < 48), "forties.notall(_1 < 49");

	is_convertible<Ref<Array>, Ref<Any> >("Ref<Array> is convertible into a Ref<Any>");
	is_inconvertible<Ref<Any>, Ref<Array> >("Ref<Any> is not convertible into a Ref<Array>");//XXX
	is_convertible<Scalar::Temp, Ref<Array> >("Ref<Array> is convertible into a Ref<Any>");
	is_inconvertible<Ref<Array>, Ref<Hash> >("Ref<Array> is not convertible into a Ref<Hash>");

	Ref<Array> ref = forties.take_ref();

	is(ref[0], 46, "ref[0] == 46");
	Array fifties = *ref;
	note("++ref[0]");
	++ref[0];
	is(ref[0], 47, "ref[0] == 47");
	is(forties[0], 47, "forties[0] == 47");
	is(fifties[0], 46, "fiftes[0] == 46");

	*ref = fifties;
	note("*ref = fifties");
	is(ref[0], 46, "ref[0] == 46");
	is(forties[0], 46, "forties[0] == 46");

	Ref<Any> last = ref;
	ok(true, "Ref<Any> last = ref");

	return exit_status();
}
