#include <iostream>
namespace TAP {
	void plan(unsigned);
	unsigned planned();
	unsigned encountered();
	void skip(unsigned);
	void print_ok(const char*);
	void ok(bool, const char*);
	void not_ok(bool, const char*);
	void print_fail(const char*);
	void skip();
	void skip_todo();
	void skip(std::string why);
	void skip_todo(std::string why);
}

using TAP::ok;
using TAP::skip;
using TAP::skip_todo;


#define TEST_START(num) {\
	const char* _current_message = NULL;\
	try {\
		TAP::plan(num)

#define TEST_END }\
		catch (...) {\
			TAP::print_fail(_current_message);\
			exit(1);\
		}\
	}

#define BLOCK_START(expected) \
	try {\
		const char* _current_message = NULL;\
		_local_expected = TAP::encountered() + expected;


#define BLOCK_END }\
	catch (...) {\
		TAP::print_fail(_current_message);\
		while (_local_expected < TAP::encountered()) {\
			skip("failure of depenancy")\
		}\
	}

#define TRY(action, name) do {\
		try {\
			action;\
			TAP::print_ok(name);\
		}\
		catch (...) {\
			TAP::print_fail(name);\
		}\
	} while (0)

#define FAIL(action, name) do {\
		try {\
			action;\
			TAP::print_fail(name);\
		}\
		catch (...) {\
			TAP::print_ok(name);\
		}\
	} while (0)

// This small macro is the whole reason for this ugly exercise. I can't introduce a new scope because code subsequent to the declaration should be visible to the rest of the code. At the same time, it must be exception safe. They are quite sever constraints :-(.
#define TRY_DECL(action, new_message) \
	_current_message = new_message;\
	action;\
	TAP::print_ok(_current_message);\
	_current_message = NULL

template<typename T, typename U> bool is(const T& left, const U& right, const char* message) {
	try {
		return left == right;
	}
	catch (...) {
		TAP::print_fail(message);
	}
}

template<typename T, typename U> bool isnt(const T& left, const U& right, const char* message) {
	try {
		return left != right;
	}
	catch (...) {
		TAP::print_fail(message);
	}
}

