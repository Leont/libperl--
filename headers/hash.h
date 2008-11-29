namespace perl {
	class lock;
	class Glob;

	/*
	 * Section Hash
	 */

	class Hash;
	namespace implementation {
		namespace hash {
			class Iterator {
				interpreter* const interp;
				HE* const iterator;
				public:
				class Key_type;
				Iterator(interpreter*, HE*);
				const Key_type key() const;
				const Scalar::Temp value() const;
				Scalar::Temp value();
				uint32_t hashcode() const;
				operator bool() const;
			};
			/*
			 * Class Hash::Value
			 * Implements a hash value
			 */
			class Value {
				protected:
				interpreter* const interp;
				HV* const handle;
				public:
				Value(interpreter*, HV*);
				Value& operator=(const Value&);


				const Scalar::Temp operator[](const Raw_string) const;
				Scalar::Temp operator[](const Raw_string);
				const Scalar::Temp operator[](const Scalar::Base&) const;
				Scalar::Temp operator[](const Scalar::Base&);

				void insert(Raw_string, const Scalar::Base&);
				void insert(const Scalar::Base&, const Scalar::Base&);

				bool exists(Raw_string) const;
				bool exists(const Scalar::Base&) const;
				const Scalar::Temp erase(Raw_string);
				const Scalar::Temp erase(const Scalar::Base&);
				void clear();
				void undefine();
				const Scalar::Temp scalar() const;
				unsigned length() const;

				const scalar::Temp_template<reference::Nonscalar<Hash> > take_ref() const;
				private:
				void foreach_init() const;
				const Iterator next_value() const;
				Iterator next_value();
				public:
				template<typename T> void each(const T& functor) const {
					foreach_init();
					while (const Iterator pair = next_value()) {
						functor(pair.key(), pair.value());
					}
				}
				template<typename T> void each(const T& functor) {
					foreach_init();
					while (Iterator pair = next_value()) {
						functor(pair.key(), pair.value());
					}
				}
				const Array::Temp keys() const;
				const Array::Temp values() const;

				void tie_to(const Scalar::Base&);
				template<typename T1, typename T2, typename T3, typename T4, typename T5> const Ref<Any>::Temp tie(const char* package_name, const T1& t1 = null_type(), const T2& t2 = null_type(), const T3& t3 = null_type(), const T4& t4 = null_type(), const T5& t5 = null_type());
				void untie(); //TODO
				const Scalar::Temp tied() const;

				static const std::string& cast_error();

				friend class perl::Hash;
				friend class perl::lock;
				friend class perl::Glob;
			};

			class Iterator::Key_type {
				const Iterator& ref;
				Key_type& operator=(const Key_type&);
				public:
				explicit Key_type(const Iterator& _ref);
				Raw_string as_raw_string() const;
				const Scalar::Temp as_scalar() const;
				operator const Raw_string() const;
				operator const Scalar::Temp() const;
				const std::string to_string() const;
			};
			std::ostream& operator<<(std::ostream&, const Iterator::Key_type&);
		}

        template<typename T> struct perl_type<T, typename boost::enable_if<typename boost::is_base_of<hash::Value, T>::type>::type> {
            typedef boost::true_type type;
        };

	}

	/*
	 * Class Hash
	 * Implements a hash variable
	 */
	class Hash : public implementation::hash::Value {
		public:
		typedef implementation::hash::Value Value;
		class Temp;

		typedef implementation::hash::Iterator Iterator;
		typedef Iterator::Key_type Key_type;

		Hash(const Hash&);
		Hash(const Temp&);
		
		~Hash();
		using Value::operator=;
		static bool is_storage_type(const Any::Temp&);
	};

	class Hash::Temp : public Value {
		friend class perl::Hash;
		mutable bool owns;
		Temp& operator=(const Temp&);
		public:
		Temp(const Temp&);
		Temp(interpreter*, HV*, bool);
		void release() const;
		~Temp();
	};
	namespace implementation {
		namespace reference {
			template<> struct type_traits<Hash> {
				typedef Hash::Temp lvalue;
				typedef HV* raw_type;
			};
			/*
			 * Class Ref<Hash>::Value
			 */
			template<> class Nonscalar<Hash> : public Nonscalar_base<Hash> {
				protected:
				Nonscalar(interpreter*, SV*);
				public:
				Scalar::Temp operator[](Raw_string index) const;
				Scalar::Temp operator[](const scalar::Base& index) const;

				static bool is_compatible_type(const Scalar::Base& var);
			};
		}
	}
}	
