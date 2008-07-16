#include "tap++.h"

namespace TAP {
	static unsigned expected = 0;
	static unsigned counter = 0;
	const char* current_message = NULL;
	void plan(int planned) {
		if (expected == 0) {
			std::cout << "1.." << planned << std::endl;
		}
		expected = planned;
	}
	int planned() {
		return expected;
	}

	void print_ok(const char* message) {
		std::cout << "ok " << ++counter << " - " << message << std::endl;
	}
	void ok(bool is_ok, const char* message) {
		const char* hot_or_not = is_ok ? "" : "not ";
		std::cout << hot_or_not << "ok " << ++counter << " - " << message << std::endl;
	}
	void not_ok(bool is_not_ok, const char* message) {
		ok(!is_not_ok, message);
	}
	void print_fail(const char* message) {
		std::cout << "not ok " << ++counter << " - " << message << std::endl;
		current_message = NULL;
	}
	void skip(int count) {
		counter += count;
	}

	void push_message(const char* new_message) {
		current_message = new_message;
	}
	void pop_message() {
		if (current_message != NULL) {
			print_ok(current_message);
		}
		current_message = NULL;
	}
	void replace_message(const char* new_message) {
		pop_message();
		push_message(new_message);
	}
	void reset_message() {
		current_message = NULL;
	}
	const char* get_message() {
		return current_message;
	}
}
