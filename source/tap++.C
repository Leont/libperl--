#define WANT_TEST_EXTRAS
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
		bool no_planned = false;
	}
	void plan(int tests) throw() {
		if (is_planned) {
			bail_out("Can't plan again!");
		}
		is_planned = true;
		*details::output << "1.." << tests << std::endl;
		expected = tests;
	}
	void plan(const details::skip_all_type&, const std::string& reason) throw() {
		*details::output << "1..0 #skip " << reason << std::endl;
		exit(0);
	}
	void plan(const details::no_plan_type&) throw() {
		is_planned = true;
		no_planned = true;
	}

	int planned() throw() {
		return expected;
	}
	int encountered() throw() {
		return counter;
	}

	int exit_status() throw () {
		if (expected == counter || no_planned) {
			return std::min(254, not_oks);
		}
		else {
			return 255;
		}
	}
	bool summary() throw() {
		return not_oks;
	}

	void bail_out(const std::string& reason) throw() {
		*details::output << "Bail out!  " << reason << std::endl;
		exit(255); // Does not unwind stack!
	}

	bool ok(bool is_ok, const std::string& message) throw() {
		const char* hot_or_not = is_ok ? "" : "not ";
		*details::output << hot_or_not << "ok " << ++counter<< " - " << message << todo_test()  << std::endl;
		if (!is_ok) {
			++not_oks;
		}
		return is_ok;
	}
	bool not_ok(bool is_not_ok, const std::string& message) throw() {
		return !ok(!is_not_ok, message);
	}

	bool pass(const std::string& message) throw() {
		return ok(true, message);
	}
	bool fail(const std::string& message) throw() {
		return ok(false, message);
	}

	void skip(int num, const std::string& reason) throw ()  {
		for(int i = 0; i < num; ++i) {
			pass(" # skip " + reason);
		}
	}

	void set_output(std::ostream& new_output) throw () {
		if (is_planned) {
			bail_out("Can't set output after plan()");
		}
		details::output = &new_output;
	}
	void set_error(std::ostream& new_error) throw() {
		if (is_planned) {
			bail_out("Can't set error after plan()");
		}
		details::error = &new_error;
	}
	namespace details {
		std::ostream* output = &std::cout;
		std::ostream* error = &std::cerr;
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
