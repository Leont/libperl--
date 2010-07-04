namespace perl {
	class lock;

	class Glob {
		interpreter* const interp;
		GV* handle;
		Glob(interpreter*, GV*);
		Glob(const Glob&);
		public:
		Glob& operator=(const Glob&);

		Glob& operator=(const Scalar::Base&);
		Glob& operator=(const Array::Value&);
		Glob& operator=(const Hash::Value&);
		Glob& operator=(const Code::Value&);
		~Glob();

		Raw_string name() const;
		Scalar::Temp scalar_value() const;
		Array::Temp array_value() const;
		Hash::Temp hash_value() const;
		Code::Value code_value() const;

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
		friend class implementation::String;
		Regex(std::auto_ptr<Implementation>);
		public:
		const implementation::scalar::Temp_template<implementation::reference::Nonscalar<Regex> > take_ref() const;
		const Array::Temp match(const String::Value&, const char* = "") const;
		const Array::Temp match(const Scalar::Value&, const char* = "") const;
		const Array::Temp match(const char*, const char* = "") const;
		const Array::Temp substitute(String::Value&, const String::Value&, const char* = "") const;
		const Array::Temp substitute(String::Value&, Raw_string, const char* = "") const;
	};

	namespace implementation {
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
