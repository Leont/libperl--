#include "tap++.h"
#include <stack>

namespace TAP {
	std::string TODO = "";

	const details::skip_all_type skip_all = details::skip_all_type();
	const details::no_plan_type no_plan = details::no_plan_type();

	namespace {
		int expected = 0;
		int counter = 0;
		int not_oks = 0;

		std::string todo_test() {
			if (TODO == "") {
				return TODO;
			}
			else {
				return " # TODO " + TODO;
			}
		}
		bool is_planned = false;
	}
	void plan(int tests) {
		if (is_planned) {
			std::cout << "Can't plan again!" << std::endl;
			exit(255);
		}
		is_planned = true;
		std::cout << "1.." << tests << std::endl;
		expected = tests;
	}
	void plan(const details::skip_all_type&, const std::string& reason) {
		std::cout << "1..0 #skip " << reason << std::endl;
		exit(0);
	}
	void plan(const details::no_plan_type&) {
		is_planned = true;
	}

	int planned() {
		return expected;
	}
	int encountered() {
		return counter;
	}

	int exit_status() {
		if (expected == counter) {
			return not_oks > 254 ? 254 : not_oks;
		}
		else {
			return 255;
		}
	}

	void bail_out(const std::string& reason) {
		std::cout << "Bail out!  " << reason << std::endl;
		exit(255); // Does not unwind stack!
	}

	bool ok(bool is_ok, const std::string& message) {
		const char* hot_or_not = is_ok ? "" : "not ";
		std::cout << hot_or_not << "ok " << ++counter<< " - " << message << todo_test()  << std::endl;
		if (!is_ok) {
			++not_oks;
		}
		return is_ok;
	}
	bool not_ok(bool is_not_ok, const std::string& message) {
		return !ok(!is_not_ok, message);
	}

	bool pass(const std::string& message) {
		return ok(true, message);
	}
	bool fail(const std::string& message) {
		return ok(false, message);
	}

	void skip(int, const std::string& reason) {
		//TODO
	}
	namespace details {
		void print_skip(const std::string &why) {
			pass(" # skip " + why);
		}
		void print_todo(const std::string& why) {
			fail(" # TODO " + why);
		}

		static std::stack<int> block_expected;
		void start_block(int expected) {
			block_expected.push(encountered() + expected);
		}
		int stop_block() {
			int ret = block_expected.top();
			block_expected.pop();
			return ret;
		}

		todo_guard::todo_guard() : value(TODO) {
		}
		todo_guard::~todo_guard() {
			TODO = value;
		}
	}
	
	void skip(const std::string& reason) {
		throw details::Skip_exception(reason);
	}
	void skip_todo(const std::string& reason) {
		throw details::Todo_exception(reason);
	}

}
