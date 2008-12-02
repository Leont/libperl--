#include "perl++.h"
#include "tap++.h"
#include "lambda.h"
#include <algorithm>
#include <map>
#include <iostream>

using namespace perl;
using namespace TAP;

void test(const Raw_string key, const Scalar::Value& value) {
	std::map<std::string, std::string> other;
	other.insert(std::pair<const std::string, std::string>("foo", "bar"));
	other.insert(std::pair<const std::string, std::string>("baz", "quz"));

	is(value, other[key.to_string()], value.operator std::string const() + " == other[" + key.to_string() + "]");
}

void print(const Raw_string key, const String::Base& value) {
	std::cout << key << ": " << value << std::endl;
}

int main(int argc, char** argv) {
	plan(32);
	Interpreter universe;
	Hash hash = universe.hash();

	ok(!hash.exists("foo"), "! hash.exists(\"foo\")");
	hash["foo"] = "bar";
	ok(hash.exists("foo"), "hash.exists(\"foo\")");
	is(hash["foo"], "bar", "hash[\"foo\"] == \"bar\"");
	ok(hash["foo"].defined(), "defined hash[\"foo\"]");

	hash.erase("foo");
	ok(!hash.exists("foo"), "! hash.exists(\"foo\")");
	hash.insert("foo", universe.value_of("bar"));
	ok(hash.exists("foo"), "hash.exists(\"foo\")");

	hash.insert("baz", universe.value_of("quz"));
	is(hash.length(), 2u, "hash.length() == 2");
	is(hash.keys().length(), 2u, "hash.keys().length() == 2");
	ok(hash.scalar(), "hash.scalar()");

	note("hash.clear()");
	hash.clear();
	is(hash.length(), 0u, "hash.length() == 0");

	hash.insert("foo", universe.value_of("bar"));
	hash.insert("baz", universe.value_of("quz"));
	is(hash.length(), 2u, "hash.length() == 2");
	note("hash.undefine()");
	hash.undefine();
	is(hash.length(), 0u, "hash.length() == 0");

	hash.insert("foo", universe.value_of("bar"));
	hash.insert("baz", universe.value_of("quz"));

	hash.each(test);

	Hash copy = hash;

	copy["foo"] = "rab";
	is(copy["foo"], "rab", "copy[\"foo\"] == \"rab\"");
	is(hash["foo"], "bar", "hash[\"foo\"] == \"bar\"");

	Ref<Hash> ref = hash.take_ref();
	ref["foo"] = "rab";

	is(ref["foo"], "rab", "ref[\"foo\"] == \"rab\"");
	is(hash["foo"], "rab", "hash[\"foo\"] == \"rab\"");

	/* Redo all tests, but with scalar keys instead of string keys */

	hash.undefine();

	String baz = universe.value_of("baz");
	String foo = universe.value_of("foo");

	ok(!hash.exists(foo), "! hash.exists(\"foo\")");
	hash[foo] = "bar";
	hash.each(print);
	ok(hash.exists(foo), "hash.exists(\"foo\")");
	is(hash[foo], "bar", "hash[\"foo\"] == \"bar\"");
	ok(hash[foo].defined(), "defined hash[\"foo\"]");

	hash.erase(foo);
	ok(!hash.exists(foo), "! hash.exists(\"foo\")");
	hash.insert(foo, universe.value_of("bar"));
	ok(hash.exists(foo), "hash.exists(\"foo\")");

	hash.insert(baz, universe.value_of("quz"));
	is(hash.length(), 2u, "hash.length() == 2");
	is(hash.keys().length(), 2u, "hash.keys().length() == 2");
	ok(hash.scalar(), "hash.scalar()");

	note("hash.clear()");
	hash.clear();
	is(hash.length(), 0u, "hash.length() == 0");

	hash.insert(foo, universe.value_of("bar"));
	hash.insert(baz, universe.value_of("quz"));
	is(hash.length(), 2u, "hash.length() == 2");
	note("hash.undefine()");
	hash.undefine();
	is(hash.length(), 0u, "hash.length() == 0");

	hash.insert(foo, universe.value_of("bar"));
	hash.insert(baz, universe.value_of("quz"));

	hash.each(test);

	return exit_status();
}
