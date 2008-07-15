#include <iostream>
namespace TAP {
	void plan(int);
	int planned();
	void skip(unsigned);
	void print_ok(const char*);
	void ok(bool, const char*);
	void not_ok(bool, const char*);
	void print_fail(const char*);
	void push_message(const char*);
	void pop_message();
	void replace_message(const char*);
	const char* get_message();
}

#define TEST_START \
	try {

#define TEST_END pop_message();\
	}\
	catch(...) {\
		print_fail(get_message());\
		std::cout << TAP::get_message();\
		std::terminate();\
	}

#define TRY(cond, name) do { try {\
	pop_message();\
	cond;\
	TAP::print_ok(name);\
}\
catch(...) {\
	TAP::print_fail(name);\
} } while (0)

template<typename T, typename U> bool is(const T& left, const U& right, const char* message) {
	try {
		return left == right, message;
	}
	catch(...) {
		TAP::print_fail(message);
	}
}
#define TRY_MESSAGE(new_message) replace_message(new_message)
