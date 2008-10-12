#include <iostream>
#include <string>
#include <boost/type_traits/is_convertible.hpp>
#include <cmath>

namespace TAP {
	namespace details {
		struct skip_all_type {};
		struct no_plan_type {};
	}
	extern const details::skip_all_type skip_all;
	extern const details::no_plan_type no_plan;
	void plan(int);
	void plan(const details::skip_all_type&, const std::string& = "");
	void plan(const details::no_plan_type&);

	int planned();
	int encountered();

	bool ok(bool, const std::string& = "");
	bool not_ok(bool, const std::string& = "");

	bool pass(const std::string& = "");
	bool fail(const std::string& = "");

	void skip(int, const std::string& = "");
	void bail_out(const std::string& reason);

	int exit_status();

	template<typename T> void diag(const T& first) {
		std::cerr << "# " << first << std::endl;
	}
	template<typename T1, typename T2> void diag(const T1& first, const T2& second) {
		std::cerr << "# " << first << second << std::endl;
	}
	template<typename T1, typename T2, typename T3> void diag(const T1& first, const T2& second, const T3& third) {
		std::cerr << "# " << first << second << third << std::endl;
	}
	template<typename T1, typename T2, typename T3, typename T4> void diag(const T1& first, const T2& second, const T3& third, const T4& fourth) {
		std::cerr << "# " << first << second << third << fourth << std::endl;
	}
	template<typename T1, typename T2, typename T3, typename T4, typename T5> void diag(const T1& first, const T2& second, const T3& third, const T4& fourth, const T5& fifth) {
		std::cerr << "# " << first << second << third << fourth << fifth << std::endl;
	}

	template<typename T, typename U> bool is(const T& left, const U& right, const std::string& message = "") {
		try {
			bool ret = ok(left == right, message);
			if (!ret) {
				diag("Failed test '", message, "'");
				diag("       Got: ", left);
				diag("  Expected: ", right);
			}
			return ret;
		}
		catch(const std::exception& e) {
			fail(message);
			diag("Failed test '", message, "'");
			diag("Cought exception '", e.what(), "'");
			diag("       Got: ", left);
			diag("  Expected: ", right);
			return false;
		}
		catch(...) {
			fail(message);
			diag("Failed test '", message, "'");
			diag("Cought unknown exception");
			diag("       Got: ", left);
			diag("  Expected: ", right);
			return false;
		}
	}

	template<typename T, typename U> bool isnt(const T& left, const U& right, const std::string& message = "") {
		try {
			return ok(left != right, message);
		}
		catch(const std::exception& e) {
			fail(message);
			diag("In test ", message);
			diag("Cought exception: ", e.what());
			return false;
		}
		catch(...) {
			fail(message);
			diag("In test ", message);
			diag("Cought unknown exception");
			return false;
		}
	}

	template<typename T, typename U> bool is_close(const T& left, const U& right, const std::string& message = "", double deviation = 0.01) {
		try {
			if(2 * fabs(left - right) / (fabs(left) + fabs(right)) < deviation) {
				pass(message);
				return true;
			}
		}
		catch(...) {}
		return fail(message);
	}

	template<typename T, typename U> bool is_remote(const T& left, const U& right, const std::string& message = "", double deviation = 0.01) {
		try {
			if(2 * fabs(left - right) / (fabs(left) + fabs(right)) > deviation) {
				pass(message);
				return true;
			}
		}
		catch(...) {}
		fail(message);
		return false;
	}

	template<typename T, typename U> bool is_convertible(const std::string& message) {
		return ok(boost::is_convertible<T, U>::value, message);
	}

	template<typename T, typename U> bool is_inconvertible(const std::string& message) {
		return ok(!boost::is_convertible<T, U>::value, message);
	}

#define WANT_TEST_EXTRAS
#ifndef WANT_TEST_EXTRAS
}
#else
	namespace details {
		struct Skip_exception {
			const std::string reason;
			Skip_exception(const std::string& _reason) : reason(_reason) {
			}
		};
		struct Todo_exception {
			const std::string reason;
			Todo_exception(const std::string& _reason) : reason(_reason) {
			}
		};
		void print_skip(const std::string&);
		void print_todo(const std::string&);

		void start_block(int);
		int stop_block();

		class todo_guard {
			const std::string value;
			public:
			todo_guard();
			~todo_guard();
		};
	}

	void skip(const std::string& reason);
	void skip_todo(const std::string& reason);
	std::string todo;
}

#define TRY(action, name) do {\
		try {\
			action;\
			TAP::pass(name);\
		}\
		catch (const std::exception& e) {\
			TAP::fail(name);\
			diag("Reported error: ", e.what());\
		}\
		catch (...) {\
			TAP::fail(name);\
		}\
	} while (0)

#define FAIL(action, name) do {\
		try {\
			action;\
			TAP::fail(name);\
		}\
		catch (...) {\
			TAP::pass(name);\
		}\
	} while (0)

#define TEST_START(num) {\
		const char* _current_message = NULL;\
		TAP::plan(num);\
		try {

#define TEST_END \
			if (TAP::encountered() < TAP::planned()) {\
				TAP::diag("Looks like you planned ", TAP::planned(), " tests but only ran ", TAP::encountered());\
				return 255;\
			}\
			else if(TAP::encountered() > TAP::planned()) {\
				TAP::diag("Looks like you planned ", TAP::planned(), " tests but ran ", TAP::encountered() - TAP::planned(), "extra");\
				return 255;\
			}\
		}\
		catch(TAP::details::Skip_exception& skipper) {\
			TAP::skip(TAP::encountered() - TAP::planned(), skipper.reason);\
		}\
		catch(TAP::details::Todo_exception& todoer) {\
			/*TODO*/\
		}\
		catch(const std::exception& e) {\
			TAP::fail(_current_message);\
			diag("Got error: ", e.what());\
			return TAP::exit_status();\
		}\
		catch (...) {\
			TAP::fail(_current_message);\
			return TAP::exit_status();\
		}\
		return TAP::exit_status();\
	}

#define BLOCK_START(planned) \
	try {\
		details::todo_guard foo##planned;\
		TAP::details::start_block(planned);


#define BLOCK_END \
		if (TAP::encountered() != TAP::details::stop_block()) {\
			diag("There seems to be a wrong number of tests!");\
		}\
	}\
	catch(TAP::details::Skip_exception& skipper) {\
		TAP::skip(TAP::encountered() - TAP::planned(), skipper.reason);\
	}\
	catch(TAP::details::Todo_exception& todoer) {\
		/*TODO*/\
	}\
	catch(const std::exception& e) {\
		TAP::fail(_current_message);\
		diag("Got error: ", e.what());\
	}\
	catch (...) {\
		TAP::fail(_current_message);\
	}

/* This small macro is a main reason for this ugly exercise. I can't introduce a new scope because
 * code subsequent to the declaration should be visible to the rest of the code. At the same time, it
 * must be exception safe. They are quite severe constraints :-(.
 */
#define TRY_DECL(action, new_message) \
	_current_message = new_message;\
	action;\
	TAP::pass(_current_message);\
	_current_message = NULL

#endif /*WANT_TEST_EXTRAS*/

using namespace TAP;
