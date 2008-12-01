namespace perl {
	namespace implementation {
		/*
		 * Class Undefined::Value
		 */
		struct Undefined : public Scalar::Base {
			Undefined(interpreter*, SV*);

			static bool is_compatible_type(const Scalar::Base&);
			static const std::string& cast_error();
		};

		/*
		 * Class Integer::Value
		 */
		class Integer : public Scalar::Base {
			Integer(const Integer&);
			protected:
			Integer(interpreter*, SV*);
			~Integer() {
			}
			public:
			
			Integer& operator=(const Integer&);
			Integer& operator=(IV);
			
			Integer& operator+=(IV);
			Integer& operator-=(IV);
			Integer& operator*=(IV);
			Integer& operator/=(IV);
			Integer& operator%=(IV);

			Integer& operator++();
			scalar::Temp_template<Integer> operator++(int);
			Integer& operator--();
			scalar::Temp_template<Integer> operator--(int);

			bool operator==(const Integer&) const;
			
			operator IV() const;
			IV int_value() const;
			static SV* copy(const Scalar::Base&);
			static bool is_compatible_type(const Scalar::Base&);
			static const std::string& cast_error();
			static SV* move(Scalar::Base&);
		};
		bool operator!=(const Integer&, const Integer&);

		/*
		 * Class Uinteger::Value
		 */
		class Uinteger : public Scalar::Base {
			Uinteger(const Uinteger&);
			protected:
			Uinteger(interpreter*, SV*);
			~Uinteger() {
			}
			public:
			
			Uinteger& operator=(const Uinteger&);
			Uinteger& operator=(UV);
			
			Uinteger& operator+=(UV);
			Uinteger& operator-=(UV);
			Uinteger& operator*=(UV);
			Uinteger& operator/=(UV);
			Uinteger& operator%=(UV);
			Uinteger& operator&=(UV);
			Uinteger& operator|=(UV);
			Uinteger& operator^=(UV);

			Uinteger& operator++();
			scalar::Temp_template<Uinteger> operator++(int);
			Uinteger& operator--();
			scalar::Temp_template<Uinteger> operator--(int);

			bool operator==(const Uinteger&) const;
			
			operator UV() const;
			UV unsigned_value() const;

			static SV* copy(const Scalar::Base&);
			static bool is_compatible_type(const Scalar::Base&);
			static const std::string& cast_error();
		};
		bool operator!=(const Uinteger&, const Uinteger&);

		/*
		 * Class Number::Value
		 */
		class Number : public Scalar::Base {
			Number(const Number&);
			protected:
			Number(interpreter*, SV*);
			~Number() {
			}
			public:
			
			Number& operator=(const Number&);
			Number& operator=(NV);
			
			Number& operator+=(NV);
			Number& operator-=(NV);
			Number& operator*=(NV);
			Number& operator/=(NV);

			operator NV() const;
			NV number_value() const;

			static SV* copy(const Scalar::Base&);
			static bool is_compatible_type(const Scalar::Base&);
			static const std::string& cast_error();
		};

		/*
		 * Class String::Value
		 */
		class String : public Scalar::Base {
			String(const String&);
			typedef void (String::*bool_type)() const;
			void no_such_comparator() const;
			protected:
			String(interpreter*, SV*);
			~String() {
			}
			public:

			operator const Raw_string() const;
			const char* get_raw() const;
//			operator const char*() const;
			operator bool_type() const;

			const std::string to_string() const;

			unsigned length() const;

			void replace(unsigned offset, unsigned length, Raw_string other);
			void replace(unsigned offset, unsigned length, const char*, unsigned);
			void insert(unsigned offset, Raw_string other);
			void insert(unsigned offset, const char*, unsigned);

			String& operator=(const String&);
			String& operator=(Raw_string other);
			String& operator=(const char*);
			String& operator=(const std::string& other);

			String& operator+=(const String&);
			String& operator+=(Raw_string);
			String& operator+=(const char*);
			String& operator+=(const std::string&);

			bool operator==(const String&) const;
			bool operator==(const std::string&) const;
			bool operator==(const char*) const;
			bool operator<(const String&) const;
			bool operator<(const std::string&) const;

			const array::Temp unpack(const Raw_string) const;
			Package get_package(bool) const;

			bool match(const perl::Regex&) const;
			bool match(Raw_string) const;
			bool substitute(const perl::Regex&, const String&);
			bool substitute(const perl::Regex&, Raw_string);

			static SV* copy(const Scalar::Base&);
			static bool is_compatible_type(const Scalar::Base&);
			static const std::string& cast_error();
		};
		bool operator==(const std::string&, const String&);
		bool operator==(const char*, const String&);

		bool operator!=(const String&,      const String&);
		bool operator!=(const String&,      const std::string&);

		bool operator!=(const std::string&, const String&);
		bool operator!=(const String&, const char*);
		bool operator!=(const char*, const String&);
	}

	typedef implementation::scalar::Variable<implementation::Undefined> Undefined;
	typedef implementation::scalar::Variable<implementation::Integer> Integer;
	typedef implementation::scalar::Variable<implementation::Uinteger> Uinteger;
	typedef implementation::scalar::Variable<implementation::Number> Number;
	typedef implementation::scalar::Variable<implementation::String> String;
}
