#include <iostream>
namespace TAP {
	void plan(unsigned);
	void skip(unsigned);
	void ok(bool, const char*);
	void not_ok(bool, const char*);
	void fail(const char*);
}

#define SANE(cond, name) do { try {\
	cond\
}\
catch(...) {\
	TAP::fail(name)\
} } while (0)
#define OK(cond, name) SANE( TAP::ok(cond, name), name)
template<typename T, typename U> bool is(const T& left, const U& right, const char* message) {
	try {
		return left == right, message;
	}
	catch(...) {
		TAP::fail(message);
	}
}
