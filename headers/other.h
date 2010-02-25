namespace perl {
	class lock;
	class Glob;

	class Handle {
		interpreter* const interp;
		IO* const handle;
		Handle(interpreter*, IO*);
		PerlIO* in_handle() const;
		PerlIO* out_handle();
		Handle& operator=(const Handle&);
		public:
		Handle(const Handle&);
		void print(const Scalar::Base&);
		void print(Raw_string);
		void print(const std::string&);
		void print(const Array::Value&);
		template<typename T> typename boost::disable_if<typename implementation::perl_type<T>::type>::type print(const T& arg) {
			const std::string value = boost::lexical_cast<std::string, T>(arg);
			print(value);
		}
		template<typename T1, typename T2> void print(const T1& t1, const T2& t2) {
			print(t1);
			print(t2);
		}
		template<typename T1, typename T2, typename T3> void print(const T1& t1, const T2& t2, const T3& t3) {
			print(t1);
			print(t2);
			print(t3);
		}
		String::Temp read(unsigned length);
		void close();
		bool eof() const;
		bool is_open() const;
		bool is_writable() const;
		bool is_readable() const;
		~Handle();

		friend class Interpreter;
		friend class Glob;
	};
	class Glob {
		interpreter* const interp;
		GV* const handle;
		Glob(interpreter*, GV*);
		Glob(const Glob&);
		public:
		Glob& operator=(const Glob&);

		Glob& operator=(const Scalar::Base&);
		Glob& operator=(const Array::Value&);
		Glob& operator=(const Hash::Value&);
		Glob& operator=(const Code::Value&);
		Glob& operator=(const Handle&);
		~Glob();

		Raw_string name() const;
		Scalar::Temp scalar_value() const;
		Array::Temp array_value() const;
		Hash::Temp hash_value() const;
		Code::Value code_value() const;
		Handle handle_value() const;

		const implementation::scalar::Temp_template< implementation::reference::Nonscalar<Glob> > take_ref() const;

		friend class Interpreter;
	};
	
	namespace implementation {
		class Regex;
	}

	class Regex {
		typedef implementation::Regex Implementation;
		boost::shared_ptr<Implementation> pattern;
		friend class Interpreter;
		Regex(std::auto_ptr<Implementation>);
		public:
		const implementation::scalar::Temp_template<implementation::reference::Nonscalar<Regex> > take_ref() const;
		int match(const String::Value&, const char* = "") const;
		int match(const Scalar::Value&, const char* = "") const;
		int match(const char*, const char* = "") const;
		const Array::Temp comb(const String::Value&, const char* = "") const;
		const Array::Temp comb(const Scalar::Value&, const char* = "") const;
		const Array::Temp comb(const char*, const char* = "") const;
		int substitute(String::Value&, const String::Value&) const;
		int substitute(String::Value&, Raw_string) const;
	};

	namespace implementation {
        template<> struct perl_type<Handle> {
            typedef boost::true_type type;
        };
        template<> struct perl_type<Glob> {
            typedef boost::true_type type;
        };
        template<> struct perl_type<Regex> {
            typedef boost::true_type type;
        };

		namespace reference {
			template<> struct type_traits<Glob> {
				typedef Glob lvalue;
				typedef GV* raw_type;
			};

			template<> class Nonscalar<Glob> : public Nonscalar_base<Glob> {
				protected:
				Nonscalar(interpreter*, SV*);
				public:
				static bool is_compatible_type(const Scalar::Base& var);
			};

			template<> struct type_traits<Handle> {
				typedef Handle lvalue;
				typedef IO* raw_type;
			};

			template<> class Nonscalar<Handle> : public Nonscalar_base<Handle> {
				protected:
				Nonscalar(interpreter*, SV*);
				public:
				static bool is_compatible_type(const Scalar::Base& var);
			};

			template<> struct type_traits<perl::Regex> {
				typedef Regex lvalue;
				typedef REGEXP* raw_type;
			};

			template<> class Nonscalar<perl::Regex> : public Nonscalar_base<perl::Regex> {
				protected:
				Nonscalar(interpreter*, SV*);
				public:
				static bool is_compatible_type(const Scalar::Base& var);
			};
		}
	}
}
