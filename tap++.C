#include "tap++.h"

namespace TAP {
	static unsigned expected = 0;
	static unsigned counter = 0;
	static unsigned not_oks = 0;
	void plan(unsigned planned) {
		if (expected == 0) {
			std::cout << "1.." << planned << std::endl;
		}
		expected = planned;
	}
	unsigned planned() {
		return expected;
	}
	unsigned encountered() {
		return counter;
	}
	unsigned failed() {
		return not_oks;
	}
	void skip() {
		print_ok(" #skip");
	}
	void skip_todo() {
		print_fail(" #TODO");
	}
	void skip(std::string why) {
		print_ok((" #skip " + why).c_str());
	}
	void skip_todo(std::string why) {
		print_fail((" #TODO " + why).c_str());
	}

	void print_ok(const char* message) {
		std::cout << "ok " << ++counter << " - " << message << std::endl;
	}
	bool ok(bool is_ok, const char* message) {
		const char* hot_or_not = is_ok ? "" : "not ";
		std::cout << hot_or_not << "ok " << ++counter << " - " << message << std::endl;
		if(!is_ok) {
			++not_oks;
		}
		return is_ok;
	}
	bool not_ok(bool is_not_ok, const char* message) {
		return !ok(!is_not_ok, message);
	}
	void print_fail(const char* message) {
		std::cout << "not ok " << ++counter << " - " << message << std::endl;
		++not_oks;
	}

	void diag(const char* information) {
		std::cout << "# " << information << std::endl;
	}
}
