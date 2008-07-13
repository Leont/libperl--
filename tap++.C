#include "tap++.h"
namespace TAP {
	struct status {
		status() {
		}
		static unsigned expected;
		static unsigned counter;
	};
	unsigned status::expected = 0;
	unsigned status::counter  = 0;
	void ok(bool is_ok, const char* message) {
		if (is_ok) {
			std::cout << status::counter++ << " ok - " << message << std::endl;
		}
		else {
			fail(message);
		}
	}
	void not_ok(bool is_not_ok, const char* message) {
		ok(!is_not_ok, message);
	}
	void fail(const char* message) {
		std::cout << status::counter++ << "not ok - " << message << std::endl;
	}
	void skip(int count) {
		status::expected += count;
	}
}
