/* 
 * Define types 
 * Doesn't do proper namespacing, perl doesn't seem to like that :-/
 */
#ifndef H_PERL
	struct interpreter;
	struct sv;
	struct av;
	struct hv;
	struct he;
	struct cv;
	struct gv;
	struct magic;
	struct mgvtbl;
#endif

namespace perl {
	typedef sv SV;
	typedef av AV;
	typedef hv HV;
	typedef he HE;
	typedef cv CV;
	typedef gv GV;
	typedef magic MAGIC;
	typedef mgvtbl MGVTBL;

	class Interpreter;
	bool operator==(const Interpreter&, const Interpreter&);

	/*
	 * Section: Exception classes
	 */
	class Runtime_exception : std::exception {
		const std::string message;
		Runtime_exception& operator=(const Runtime_exception& other);
		public:
		Runtime_exception(const char* _message) : std::exception(), message(_message) {
		}
		Runtime_exception(const char* _message, unsigned length) : std::exception(), message(_message, length) {
		}
		Runtime_exception(const std::string& _message) : std::exception(), message(_message) {
		}
		Runtime_exception(const Runtime_exception& other) : std::exception(), message(other.message) {
		}
		const char* what() {
			return message.c_str();
		}
		~Runtime_exception() throw() {
		}
	};

	class Out_of_bounds_exception : public Runtime_exception {
		public:
		Out_of_bounds_exception(const std::string& _message) : Runtime_exception(_message) {
		}
		Out_of_bounds_exception(const char* _message = "Out of bounds") : Runtime_exception(_message) {
		}
	};

	class Cast_exception : public Runtime_exception {
		public:
		Cast_exception(const std::string& _message) : Runtime_exception(_message) {
		}
		Cast_exception(const char* _message) : Runtime_exception(_message) {
		}
	};

	class Not_an_object_exception : public Runtime_exception {
		public:
		Not_an_object_exception() : Runtime_exception("This is not an Object") {
		}
	};

	class No_such_object_exception : public Runtime_exception {
		public:
		No_such_object_exception(const std::string& _message) : Runtime_exception(_message) {
		}
		No_such_object_exception(const char* _message = "No such Object") : Runtime_exception(_message) {
		}
	};

	/*
	 * Assertion templates
	 */
	template<bool debug, typename T> void assertion(bool condition) {
		if (debug && !condition) {
			throw T();
		}
	}
	template<bool debug, typename T, typename U> void assertion(bool condition, const U& arg) {
		if (debug && !condition) {
			throw T(arg);
		}
	}

	template<typename T> void assertion(bool condition) {
		assertion<true, T>(condition);
	}

	template<typename T, typename U> void assertion(bool condition, const U& arg) {
		assertion<true, T>(condition, arg);
	}

	/*
	 * Class Raw_string.
	 * A wrapper around a string pointer. Knows its length.
	 * TODO: proper uft-8 handling.
	 */
	struct Raw_string {
		const char* const value;
		const unsigned length;
		const bool utf8;
		Raw_string(const char* value);
		Raw_string(const char* value, unsigned length, bool uft8);
		Raw_string(const std::string& val);
		operator const char*() const;
		std::string to_string() const;
	};
	int raw_cmp(const Raw_string left, const Raw_string right);
	bool operator==(const Raw_string, const Raw_string);
	bool operator!=(const Raw_string, const Raw_string);
	bool operator<( const Raw_string, const Raw_string);
	bool operator>( const Raw_string, const Raw_string);
	bool operator<=(const Raw_string, const Raw_string);
	bool operator>=(const Raw_string, const Raw_string);

	std::ostream& operator<<(std::ostream& stream, const Raw_string data);

	struct override { };

	template<typename T, typename U> class this_binder {
		T& object;
		const U& method;
		public:
		this_binder(T& _object, const void (T::* _method)(const U&)) : object(_object), method(_method) {
		}
		template<typename V> void operator()(const V&) {
		}
	};
	namespace implementation {
		struct null_type { };
	}
}
