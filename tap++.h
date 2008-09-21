#include <iostream>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/mpl/not.hpp>

namespace TAP {
	void plan(unsigned);
	unsigned planned();
	unsigned encountered();
	unsigned failed();

	void skip(unsigned);
	void print_ok(const char*);
	void print_fail(const char*);

	bool ok(bool, const char*);
	bool not_ok(bool, const char*);
	void skip();
	void skip_todo();
	void skip(std::string why);
	void skip_todo(std::string why);
	void diag(const char* information);

	template<typename T> struct is_convertible {
		static void test(const char* message) {
			print_fail(message);
		}
	};
	template<> struct is_convertible<boost::true_type> {
		static void test(const char* message) {
			print_ok(message);
		}
	};
}

using TAP::ok;
using TAP::not_ok;
using TAP::skip;
using TAP::skip_todo;
using TAP::diag;


#define TEST_START(num) {\
		const char* _current_message = NULL;\
		try {\
			TAP::plan(num)

#define TEST_END \
			if (TAP::encountered() != TAP::planned()) {\
				diag("It seems the number of done tests doesn't matched the number of planned tests");\
			}\
		}\
		catch (...) {\
			TAP::print_fail(_current_message);\
			return TAP::failed();\
		}\
	return 0;\
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
		if(left == right) {
			TAP::print_ok(message);
			return true;
		}
	}
	catch(...) {}
	TAP::print_fail(message);
	return false;
}

template<typename T, typename U> bool isnt(const T& left, const U& right, const char* message) {
	try {
		if(left != right) {
			TAP::print_ok(message);
			return true;
		}
	}
	catch(...) {}
	TAP::print_fail(message);
	return false;
}

template<typename T, typename U> bool is_convertible(const char* message) {
	TAP::is_convertible<typename boost::is_convertible<T, U>::type>::test(message);
	return boost::is_convertible<T, U>::value;
}

template<typename T, typename U> bool is_inconvertible(const char* message) {
	TAP::is_convertible<typename boost::mpl::not_<typename boost::is_convertible<T, U>::type>::type>::test(message);
	return boost::is_convertible<T, U>::value;
}
