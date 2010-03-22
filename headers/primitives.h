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
		class Integer : public Scalar::Base, public scalar::Referencable<Integer> {
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
			using scalar::Referencable<Integer>::take_ref;
		};
		bool operator!=(const Integer&, const Integer&);

		/*
		 * Class Uinteger::Value
		 */
		class Uinteger : public Scalar::Base, public scalar::Referencable<Uinteger> {
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
			using scalar::Referencable<Uinteger>::take_ref;
		};
		bool operator!=(const Uinteger&, const Uinteger&);

		/*
		 * Class Number::Value
		 */
		class Number : public Scalar::Base, public scalar::Referencable<Number> {
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
			using scalar::Referencable<Number>::take_ref;
		};

		/*
		 * Class String::Value
		 */
		class String : public Scalar::Base, public scalar::Referencable<String> {
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

			size_t length() const;
			void grow(size_t);

			void replace(size_t offset, size_t length, Raw_string other);
			void replace(size_t offset, size_t length, const char*, size_t);
			void insert(size_t offset, Raw_string other);
			void insert(size_t offset, const char*, size_t);

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
			using scalar::Referencable<String>::take_ref;
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
