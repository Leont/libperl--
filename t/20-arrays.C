#include "perl++.h"
#include "tap++.h"
#include "lambda.h"
#include <algorithm>

using namespace perl;

int main(int argc, char** argv) {
	TEST_START(55);
	Interpreter universe;
	TRY_DECL(Array array = universe.list(), "array = ()");
	is(array.length(), 0u, "array.length() == 0");

	array.push(1);
	diag("array.push(1)");
	is(array[0], 1, "array[0] == 1");
	is(array[0], "1", "array[0] == \"1\"");

	++array[0];
	diag("++array[0]");
	is(array[0], 2, "array[0] == 1");
	is(array[0], "2", "array[0] == \"1\"");

	diag("array.push(UINT_MAX)");
	array.push(UINT_MAX);
	is(array[1], UINT_MAX, "array[1] == UINT_MAX");
	ok(array[1] > static_cast<UV>(INT_MAX), "array[1] > (unsigned)INT_MAX");
	is(array[1], -1, "array[1] == -1");

	array.push(300E30);
	diag("array.push(300E30)");
	is_close(array[2], 300E30, "array[2] == 300E30");

	array.push("test");
	diag("array.push(\"test\")");
	is(array[3], "test", "array[3] == \"test\"");

	array.push(Raw_string("test"));
	diag("array.push(Raw_string(\"test\"))");
	is(array[4], "test", "array[4] == \"test\"");

	array.push(universe.value_of(1));
	diag("array.push(universe.value_of(1))");
	is(array[5], 1, "array[4] == 1");

	array.clear();
	diag("array.clear()");
	is(array.length(), 0u, "array.length() == 0");
	array.push(1, UINT_MAX, 300E30, "test", universe.undef());
	diag("array.push(1, UINT_MAX, 300E30, \"test\")");

	is(array[0], 1, "array[0] == 1");
	is(array[1], UINT_MAX, "array[1] == UINT_MAX");
	is_close(array[2], 300E30, "array[2] == 300E30");
	is(array[3], "test", "array[3] == \"test\"");
	ok(array.exists(4), "exists array[4]");
	not_ok(array[4].defined(), "not: defined array[4]");

	array.push(array);
	diag("array.push(array)");
	is(array[5], 1, "array[5] == 1");
	is(array[6], UINT_MAX, "array[6] == UINT_MAX");
	is_close(array[7], 300E30, "array[7] == 300E30");
	is(array[8], "test", "array[8] == \"test\"");
	ok(array.exists(9), "exists array[9]");
	not_ok(array[9].defined(), "not: defined array[9]");

	array.clear();
	array.unshift("test");
	array.unshift(300E30);
	array.unshift(UINT_MAX);
	array.unshift(1);
	diag("array.unshift(1, UINT_MAX, 300E30, \"test\")");

	is(array[0], 1, "array[0] == 1");
	is(array[1], UINT_MAX, "array[1] == UINT_MAX");
	is_close(array[2], 300E30, "array[2] == 300E30");
	is(array[3], "test", "array[3] == \"test\"");

	ok(array.exists(3), "exists array[3]");
	String string = array.remove(3);
	diag("string = array.remove(3)");
	ok(string.length(), "string.length()");
	not_ok(array.exists(3), "not: exists array[3]");
	is(array.length(), 3u, "array.length() == 3");

	array.length() = 2;
	diag("array.length() = 2");
	not_ok(array.exists(2), "not: exists array[2]");
	is(array.length(), 2u, "array.length() == 2");
	array.extend(4);
	diag("array.extend(4)");
	is(array.length(), 2u, "array.length == 2");

	Array numbers = universe.list(1, 2, 3, 4);
	Array squares = numbers.map(ll_static_cast<IV>(_1) * _1);
	for (int i = 0; i < 4; ++i) { 
		std::string num = boost::lexical_cast<std::string>(i);
		is(squares[i], (IV)(numbers[i]) * numbers[i], (std::string("squares[") + num + "] is numbers[" + num + "] ** 2").c_str());
	}

	Array big = squares.grep(_1 > 8);
	diag("Array big = squares.grep(_1 > 8)");
	is(big.length(), 2u, "big.length == 2");
	is(big[0], 9, "big[0] == 9");

	Array doubles = numbers.map(_1 * 2);
	is(doubles[3], 8, "numbers.map(_1 * 2)[3] == 8");

	IV sum = squares.reduce(_1 + _2, 0);
	diag("sum = squares.reduce(_1 + _2, 0u)");
	is(sum, 30, "sum == 30");

	big.each(_1 *= 2);
	diag("big.each(_1 *= 2)");
	is(big[0], 18, "big[0] == 18");

	Array const forties = universe.list(47, 48, 49, 50);
	ll_is(_1, bind<int>(&TAP::encountered), "encountered == 45")(make_const(46));
	std::for_each(forties.begin(), forties.end(), ll_is(_1, bind(&TAP::encountered), "encountered == 4x"));

	ok(forties.any(_1 == 49), "forties.any(_1 == 48)");
	ok(forties.all(_1 > 45), "forties.all(_1 > 45)");
	ok(forties.none(_1 == 30), "forties.none(_1 == 30)");
	ok(forties.notall(_1 < 49), "forties.notall(_1 < 49");
	TEST_END;
}
