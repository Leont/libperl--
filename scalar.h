namespace perl {
	/*
	 * Class Any
	 * Just a placeholder
	 */
	class Any {
		Any();
		public:
		/*
		 * Class Any::Temp
		 * This class is only used for references. It is not to be used by end users.
		 */
		class Temp {
			public:
			interpreter* const interp;
			void* const handle;

			Temp(interpreter* _interp, void* _value) : interp(_interp), handle(_value) {
			}
			Temp(const Temp& other) : interp(other.interp), handle(other.handle) {
			}
		};
	};

	class Scalar;

	namespace implementation {
		bool is_this_type(const Any::Temp& var, unsigned int type);

		namespace reference {
			template<typename T> class Nonscalar;
			template<typename T> class Scalar_ref;
			template<typename T, typename Enable = void> class ref;
			class Reference_base;
		}

		namespace scalar {
			class Value;
			template<typename T> class Temp_template;
			typedef Temp_template<Value> Temp;
			/*
			 * Class Scalar::Base
			 * This class represents scalar values. It's a thin interface, as different kinds of scalar
			 * values have little in common.
			 */
			class Base {
				Base(const Base&);
				Base& operator=(const Base&);
				protected:
				Base(interpreter*, SV*);
				public:
				interpreter* const interp;
				private:
				SV* const handle;
				public:

				IV int_value() const;
				UV uint_value() const;
				NV number_value() const;
				Raw_string string_value() const;

				SV* get_SV(bool) const;
				bool is_tainted() const;
				void taint();
				void tie_to(const Base& tier);
				const Temp_template<Value> tied() const;
				void untie();
				template<typename T1, typename T2, typename T3, typename T4, typename T5> const Temp_template<reference::Nonscalar<Any> > tie(const char* package_name, const T1& t1 = null_type(), const T2& t2 = null_type(), const T3& t3 = null_type(), const T4& t4 = null_type(), const T5& t5 = null_type());
				const Temp_template< reference::Nonscalar<Any> > take_ref() const;

				protected:
				static SV* copy_sv(const Base&);
				public:
				static bool is_compatible_type(const scalar::Base&); // NOOP
				static const std::string& cast_error();
			};
		}

		template<typename T> struct perl_type<T, typename boost::enable_if<typename boost::is_base_of<scalar::Base, T>::type>::type> {
			typedef boost::true_type type;
		};

		namespace helper {
			const Any::Temp dereference(const scalar::Base&);
			void decrement(const scalar::Base&);
			void set_scalar(scalar::Base&, const scalar::Base&);
			void mortalize(const scalar::Base&);
			void share(const scalar::Base&);
			SV* take_ref(const scalar::Base&);
		}

		namespace scalar {
			/*
			 * A template to generate X::Temp
			 */
			template<typename T> class Temp_template : public T {
				typedef T Base_type;
				mutable bool owns;
				bool transferable;
				template<typename U> struct accept_anything {
					typedef typename boost::is_same<T, Value>::type type;
				};
				public:
				Temp_template(interpreter* _interp, SV* _value, bool _owns) : Base_type(_interp, _value), owns(_owns), transferable(_owns) {
				}
				Temp_template(interpreter* _interp, SV* _value, bool _owns, bool _transferable) : Base_type(_interp, _value), owns(_owns), transferable(_transferable) {
				}
				Temp_template(const Temp_template& other) : Base_type(other.interp, other.get_SV(false)), owns(other.owns), transferable(other.transferable) {
					other.owns = false;
				}
				template<typename U> friend class Temp_template;
				template<typename U> Temp_template(const Temp_template<U>& other, typename boost::enable_if<typename accept_anything<U>::type, int>::type selector = 1) : Base_type(other.interp, other.get_SV(false)), owns(other.owns), transferable(other.transferable) {
					other.owns = false;
				}
				template<typename U> Temp_template(const Temp_template<U>& other, const override&) : Base_type(other.interp, other.get_SV(false)), owns(other.owns), transferable(other.transferable) {
					other.owns = false;
				}
				Temp_template& operator=(const Temp_template& other) {
					Base_type::operator=(other);
					return *this;
				}
				SV* release(bool getmagic = true) const {
					owns = false;
					return T::get_SV(getmagic);
				}
				void mortalize() const {
					if (owns) {
						helper::mortalize(*this);
						owns = false;
					}
				}
				bool has_ownership() const {
					return owns && transferable;
				}
				~Temp_template() {
					if (owns) {
						helper::decrement(*this);
					}
				}
				using T::operator=;
				const scalar::Temp_template<typename reference::Scalar_ref<T> > take_ref() const {
					return scalar::Temp_template<typename reference::Scalar_ref<T> >(T::interp, helper::take_ref(*this), true);
				}
			};

			class Value;
			typedef Temp_template<Value> Temp;
		}
	}

	class Code {
		public:
		/*
		 * Class Code::Value
		 * Represents a subroutine. Normally only used in references.
		 */
		class Value {
			interpreter* const interp;
			CV* const handle;

			public:
			Value(interpreter*, CV*);
			const implementation::scalar::Temp_template<implementation::reference::Nonscalar<Code> > take_ref() const;
			friend class Glob;
		};
		static bool is_storage_type(const Any::Temp&);
		static const std::string& cast_error();
	};

	class Package;
	class Regex;
	enum context {VOID, SCALAR, LIST};

	namespace implementation {
		namespace array {
			class Value;
			class Temp;
		}
		class String;
		class Regex;

		/*
		 * meta-function nearest_arithmetic_type
		 * Returns the nearest match for any kind of integer
		 */
		template<typename T, typename Enable1 = void, typename Enable2 = void> struct nearest_arithmetic_type {
//		template<typename T> struct nearest_arithmetic_type<T, typename boost::enable_if<typename boost::is_integral<T>::type>::type, typename boost::enable_if<typename boost::is_signed<T>::type>::type> {
			typedef IV type;
		};
		template<typename T> struct nearest_arithmetic_type<T, typename boost::enable_if<typename boost::is_integral<T>::type>::type, typename boost::enable_if<typename boost::is_unsigned<T>::type>::type> {
			typedef UV type;
		};
		template<typename T> struct nearest_arithmetic_type<T, typename boost::enable_if<typename boost::is_floating_point<T>::type>::type, void> {
			typedef NV type;
		};
	
		/*
		 * class Perl_stack
		 * An abstraction around perl stack. Here be dragons.
		 */
		class Perl_stack {
			Perl_stack& operator=(const Perl_stack&);
			Perl_stack(const Perl_stack&);
			protected:
			Perl_stack(interpreter* _interp);
			interpreter* const interp;
			SV** sp; //very non-const ;-)
			public:
			void push(IV);
			void push(UV);
			void push(NV);
			void push(Raw_string);
			template<typename T> typename boost::enable_if<typename boost::is_arithmetic<T>::type>::type push(T value) {
				push(static_cast<typename nearest_arithmetic_type<T>::type>(value));
			}
			void push(const char*);
			void push(const std::string&);
			void push(const scalar::Base&);
			void push(const scalar::Temp&);
			void push(const implementation::array::Value& val);
			void push(const null_type&);
			void push(const Regex&);
//			template<typename T> void push(T* arg);
		};

		/*
		 * Class Call_stack
		 * A specialization of the perl stack for calling other functions. Not to be used directly.
		 */
		class Call_stack : public Perl_stack {
			void prepare_call();
			void finish_call();
			void unwind_stack(int);

			int sub_call(const char*, intptr_t);
			int sub_call(SV*, intptr_t);
			int method_call(const char*, intptr_t);

			SV* pop();
			AV* pop_array(int);
			public:
			explicit Call_stack(interpreter*);

			const scalar::Temp method_scalar(const char* name);
			const array::Temp method_array(const char* name);
			const scalar::Temp sub_scalar(const char* name);
			const scalar::Temp sub_scalar(const implementation::reference::Nonscalar<Code>& ref);
			const scalar::Temp sub_scalar(const scalar::Value& ref); //TODO
			const array::Temp sub_array(const char* name);
			const array::Temp sub_array(const implementation::reference::Nonscalar<Code>& ref);
			const array::Temp sub_array(const scalar::Value& ref); //TODO

			const scalar::Temp_template<implementation::String> pack(const Raw_string pattern);
			const array::Temp unpack(const Raw_string pattern, const Raw_string value);

			const scalar::Temp eval_scalar(SV*);
			const array::Temp eval_list(SV*);

			~Call_stack();
			template<typename T> Call_stack& push(const T& t) {
				Perl_stack::push(t);
				return *this;
			}
			
			template<typename T1, typename T2> Call_stack& push(const T1& t1, const T2& t2) {
				return push(t1).push(t2);
			}
			template<typename T1, typename T2, typename T3> Call_stack& push(const T1& t1, const T2& t2, const T3& t3) {
				return push(t1).push(t2).push(t3);
			}
			template<typename T1, typename T2, typename T3, typename T4> Call_stack& push(const T1& t1, const T2& t2, const T3& t3, const T4& t4) {
				return push(t1).push(t2).push(t3).push(t4);
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5> Call_stack& push(const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5) {
				return push(t1).push(t2).push(t3).push(t4).push(t5);
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6> Call_stack& push(const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6) {
				return push(t1).push(t2).push(t3).push(t4).push(t5).push(t6);
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7> Call_stack& push(const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7 t7) {
				return push(t1).push(t2).push(t3).push(t4).push(t5).push(t6).push(t7);
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8> Call_stack& push(const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7, const T8& t8) {
				return push(t1).push(t2).push(t3).push(t4).push(t5).push(t6).push(t7).push(t8);
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9> Call_stack& push(const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7, const T8& t8, const T9& t9) {
				return push(t1).push(t2).push(t3).push(t4).push(t5).push(t6).push(t7).push(t8).push(t9);
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10> Call_stack& push(const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7, const T8& t8, const T9& t9, const T10& t10) {
				return push(t1).push(t2).push(t3).push(t4).push(t5).push(t6).push(t7).push(t8).push(t9).push(t10);
			}
		};

		/*
		 * This metafunction serves to postpone class resulution to the moment where
		 */
		template<typename T> class return_type {
			public:
			typedef const scalar::Temp scalar_type;
			typedef const array::Temp array_type;
			typedef const scalar::Temp_template<reference::Nonscalar<Code> > code_type;
		};

		/*
		 * Class function calling.
		 * A mixin to implement function calling. Not to be used directly.
		 */
		template<class T> class function_calling {
			const T& self() const {
				return *static_cast<const T*>(this);
			}
			typedef typename implementation::return_type<T>::scalar_type scalar_type;
			typedef typename implementation::return_type<T>::array_type array_type;
			public:
			const scalar_type operator()() const {
				return implementation::Call_stack(self().interp).sub_scalar(self());
			}
			template<typename T1> const scalar_type operator()(const T1& t1) const {
				return implementation::Call_stack(self().interp).push(t1).sub_scalar(self());
			}
			template<typename T1, typename T2> const scalar_type operator()(const T1& t1, const T2& t2) const {
				return implementation::Call_stack(self().interp).push(t1, t2).sub_scalar(self());
			}
			template<typename T1, typename T2, typename T3> const scalar_type operator()(const T1& t1, const T2& t2, const T3& t3) const {
				return implementation::Call_stack(self().interp).push(t1, t2, t3).sub_scalar(self());
			}
			template<typename T1, typename T2, typename T3, typename T4> const scalar_type operator()(const T1& t1, const T2& t2, const T3& t3, const T4& t4) const {
				return implementation::Call_stack(self().interp).push(t1, t2, t3, t4).sub_scalar(self());
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5> const scalar_type operator()(const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5) const {
				return implementation::Call_stack(self().interp).push(t1, t2, t3, t4, t5).sub_scalar(self());
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6> const scalar_type operator()(const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6) const {
				return implementation::Call_stack(self().interp).push(t1, t2, t3, t4, t5, t6).sub_scalar(self());
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7> const scalar_type operator()(const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7) const {
				return implementation::Call_stack(self().interp).push(t1, t2, t3, t4, t5, t6).sub_scalar(self());
			}
			//TODO: call_array
		};

		/*
		 * Stash
		 * A helper class of method_calling
		 */

		class Stash {
			interpreter* interp;
			HV* stash;
			public: 
			Stash(const Package&);
			Stash(const reference::Reference_base&);
			Stash(const scalar::Value&);
			bool can(Raw_string) const;
			const scalar::Temp_template< reference::Nonscalar<Code> > get_method(Raw_string) const;
		};
		/*
		 * Class method_calling.
		 * A mixin to implement method calling. Not to be used directly.
		 */
		template<typename T> class method_calling {
			const T& self() const {
				return *static_cast<const T*>(this);
			}
			typedef typename implementation::return_type<T>::scalar_type scalar_type;
			typedef typename implementation::return_type<T>::array_type array_type;
			typedef typename implementation::return_type<T>::code_type code_type;
			public:
			bool can(Raw_string name) const {
				return Stash(self()).can(name);
			}
			code_type get_method(Raw_string name) const {
				return Stash(self).get_method(name);
			}
			scalar_type call(const char * name) const {
				return implementation::Call_stack(self().interp).push(self()).method_scalar(name);
			}
			template<typename T1> scalar_type call(const char* name, const T1& t1) const {
				return implementation::Call_stack(self().interp).push(self(), t1).method_scalar(name);
			}
			template<typename T1, typename T2> scalar_type call(const char* name, const T1& t1, const T2& t2) const {
				return implementation::Call_stack(self().interp).push(self(), t1, t2).method_scalar(name);
			}
			template<typename T1, typename T2, typename T3> scalar_type call(const char* name, const T1& t1, const T2& t2, const T3& t3) const {
				return implementation::Call_stack(self().interp).push(self(), t1, t2, t3).method_scalar(name);
			}
			template<typename T1, typename T2, typename T3, typename T4> scalar_type call(const char* name, const T1& t1, const T2& t2, const T3& t3, const T4& t4) const {
				return implementation::Call_stack(self().interp).push(self(), t1, t2, t3, t4).method_scalar(name);
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5> scalar_type call(const char* name, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5) const {
				return implementation::Call_stack(self().interp).push(self(), t1, t2, t3, t4, t5).method_scalar(name);
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6> scalar_type call(const char* name, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6) const {
				return implementation::Call_stack(self().interp).push(self(), t1, t2, t3, t4, t5, t6).method_scalar(name);
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7> scalar_type call(const char* name, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7) const {
				return implementation::Call_stack(self().interp).push(self(), t1, t2, t3, t4, t5, t6, t7).method_scalar(name);
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8> scalar_type call(const char* name, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7, const T8& t8) const {
				return implementation::Call_stack(self().interp).push(self(), t1, t2, t3, t4, t5, t6, t7, t8).method_scalar(name);
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9> scalar_type call(const char* name, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7, const T8& t8, const T9& t9) const {
				return implementation::Call_stack(self().interp).push(self(), t1, t2, t3, t4, t5, t6, t7, t8, t9).method_scalar(name);
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10> scalar_type call(const char* name, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7, const T8& t8, const T9& t9, const T10& t10) const {
				return implementation::Call_stack(self().interp).push(self(), t1, t2, t3, t4, t5, t6, t7, t8, t9, t10).method_scalar(name);
			}

			array_type call_array(const char* name) const {
				return implementation::Call_stack(self().interp).push(self()).method_array(name);
			}
			template<typename T1> array_type call_array(const char* name, const T1& t1) const {
				return implementation::Call_stack(self().interp).push(self(), t1).method_array(name);
			}
			template<typename T1, typename T2> array_type call_array(const char* name, const T1& t1, const T2& t2) const {
				return implementation::Call_stack(self().interp).push(self(), t1, t2).method_array(name);
			}
			template<typename T1, typename T2, typename T3> array_type call_array(const char* name, const T1& t1, const T2& t2, const T3& t3) const {
				return implementation::Call_stack(self().interp).push(self(), t1, t2, t3).method_array(name);
			}
			template<typename T1, typename T2, typename T3, typename T4> array_type call_array(const char* name, const T1& t1, const T2& t2, const T3& t3, const T4& t4) const {
				return implementation::Call_stack(self().interp).push(self(), t1, t2, t3, t4).method_array(name);
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5> array_type call_array(const char* name, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5) const {
				return implementation::Call_stack(self().interp).push(self(), t1, t2, t3, t4, t5).method_array(name);
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6> array_type call_array(const char* name, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6) const {
				return implementation::Call_stack(self().interp).push(self(), t1, t2, t3, t4, t5, t6).method_array(name);
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7> array_type call_array(const char* name, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7) const {
				return implementation::Call_stack(self().interp).push(self(), t1, t2, t3, t4, t5, t6, t7).method_array(name);
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8> array_type call_array(const char* name, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7, const T8& t8) const {
				return implementation::Call_stack(self().interp).push(self(), t1, t2, t3, t4, t5, t6, t7, t8).method_array(name);
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9> array_type call_array(const char* name, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7, const T8& t8, const T9& t9) const {
				return implementation::Call_stack(self().interp).push(self(), t1, t2, t3, t4, t5, t6, t7, t8, t9).method_array(name);
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10> array_type call_array(const char* name, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7, const T8& t8, const T9& t9, const T10& t10) const {
				return implementation::Call_stack(self().interp).push(self(), t1, t2, t3, t4, t5, t6, t7, t8, t9, t10).method_array(name);
			}
		};

		namespace hash {
			class String_callback;
			class Scalar_callback;
		}

		class String;

		namespace scalar {
			template<typename T> class Assignable;

			/*
			 * Class Scalar::Value
			 * This class can be converted into anything any scalar can be converted in. If the the convertion 
			 * fails, it throws an exception. It can also be used as a reference, but the same caveat applies.
			 */
			class Value: public Base, public implementation::function_calling<Value>, public implementation::method_calling<Value> {
				protected:
				Value(interpreter*, SV*);
				public:
				Value& operator=(const Base&);
				Value& operator=(const Value&);
				Value& operator=(IV);
				Value& operator=(UV);
				Value& operator=(NV);
				Value& operator=(Raw_string);
				Value& operator=(const std::string&);
				Value& operator=(const char*);

				template<typename T> typename boost::enable_if<typename boost::is_arithmetic<T>::type, Value&>::type operator=(T right) {
					return operator=(static_cast<typename nearest_arithmetic_type<T>::type>(right));
				}

				template<typename T> typename boost::enable_if<typename boost::is_arithmetic<T>::type, Value&>::type operator+=(T right) {
					return *this = static_cast<typename nearest_arithmetic_type<T>::type>(*this) + right;
				}
				template<typename T> typename boost::enable_if<typename boost::is_arithmetic<T>::type, Value&>::type operator-=(T right) {
					return *this = static_cast<typename nearest_arithmetic_type<T>::type>(*this) - right;
				}
				template<typename T> typename boost::enable_if<typename boost::is_arithmetic<T>::type, Value&>::type operator*=(T right) {
					return *this = static_cast<typename nearest_arithmetic_type<T>::type>(*this) * right;
				}
				template<typename T> typename boost::enable_if<typename boost::is_arithmetic<T>::type, Value&>::type operator/=(T right) {
					return *this = static_cast<typename nearest_arithmetic_type<T>::type>(*this) / right;
				}
				template<typename T> typename boost::enable_if<typename boost::is_arithmetic<T>::type, Value&>::type operator%=(T right) {
					return *this = static_cast<typename nearest_arithmetic_type<T>::type>(*this) % right;
				}

				//Begin of TODO
				Value& operator+=(const implementation::String&);
				Value& operator+=(Raw_string);
				Value& operator+=(const char*);
				Value& operator+=(const std::string&);
				// End of TODO

				Value& operator++();
				Value& operator--();

				bool is_defined() const;

				operator int() const;
				operator unsigned int() const;
				operator long() const;
				operator long long() const;
				operator unsigned long() const;
				operator short() const;
				operator unsigned short() const;
				operator unsigned long long() const;
				operator double() const;
				operator float() const;
				operator long double() const;
				operator Raw_string() const;
//				operator const char*() const;
				operator bool() const;
				bool as_bool() const;

				bool defined() const;
				unsigned length() const;
				const array::Temp unpack(const Raw_string) const;
			
				scalar::Temp operator[](int) const;
				scalar::Temp operator[](Raw_string index) const;
				scalar::Temp operator[](const Base& index) const;

				bool is_object() const;
				bool isa(const char*) const;
				bool is_exactly(const char*) const;
				const char* get_classname() const;

				static SV* copy(const Base&);
			};

			bool operator==(const Value&, IV);
			bool operator==(const Value&, UV);
			bool operator==(const Value&, NV);
			bool operator==(const Value&, const char*);
			bool operator==(const Value&, Raw_string);
			bool operator==(const Value&, const std::string&);

			template<typename T> typename boost::enable_if<typename boost::is_arithmetic<T>::type, bool>::type operator==(const Value& left, T right) {
				return static_cast<typename nearest_arithmetic_type<T>::type>(left) == right;
			}
			template<typename T> typename boost::enable_if<typename boost::is_arithmetic<T>::type, bool>::type operator!=(const Value& left, T right) {
				return static_cast<typename nearest_arithmetic_type<T>::type>(left) != right;
			}
			template<typename T> typename boost::enable_if<typename boost::is_arithmetic<T>::type, bool>::type operator<=(const Value& left, T right) {
				return static_cast<typename nearest_arithmetic_type<T>::type>(left) <= right;
			}
			template<typename T> typename boost::enable_if<typename boost::is_arithmetic<T>::type, bool>::type operator>=(const Value& left, T right) {
				return static_cast<typename nearest_arithmetic_type<T>::type>(left) >= right;
			}
			template<typename T> typename boost::enable_if<typename boost::is_arithmetic<T>::type, bool>::type operator<(const Value& left, T right) {
				return static_cast<typename nearest_arithmetic_type<T>::type>(left) < right;
			}
			template<typename T> typename boost::enable_if<typename boost::is_arithmetic<T>::type, bool>::type operator>(const Value& left, T right) {
				return static_cast<typename nearest_arithmetic_type<T>::type>(left) > right;
			}

			template<typename T> typename boost::enable_if<typename boost::is_arithmetic<T>::type, bool>::type operator==(T left, const Value& right) {
				return left == static_cast<typename nearest_arithmetic_type<T>::type>(right);
			}
			template<typename T> typename boost::enable_if<typename boost::is_arithmetic<T>::type, bool>::type operator!=(T left, const Value& right) {
				return left != static_cast<typename nearest_arithmetic_type<T>::type>(right);
			}
			template<typename T> typename boost::enable_if<typename boost::is_arithmetic<T>::type, bool>::type operator<=(T left, const Value& right) {
				return left <= static_cast<typename nearest_arithmetic_type<T>::type>(right);
			}
			template<typename T> typename boost::enable_if<typename boost::is_arithmetic<T>::type, bool>::type operator>=(T left, const Value& right) {
				return left >= static_cast<typename nearest_arithmetic_type<T>::type>(right);
			}
			template<typename T> typename boost::enable_if<typename boost::is_arithmetic<T>::type, bool>::type operator<(T left, const Value& right) {
				return left < static_cast<typename nearest_arithmetic_type<T>::type>(right);
			}
			template<typename T> typename boost::enable_if<typename boost::is_arithmetic<T>::type, bool>::type operator>(T left, const Value& right) {
				return left > static_cast<typename nearest_arithmetic_type<T>::type>(right);
			}

			template<typename T> typename nearest_arithmetic_type<T>::type operator+(const Value& left, T right) {
				return static_cast<typename nearest_arithmetic_type<T>::type>(left) + right;
			}
			template<typename T> typename nearest_arithmetic_type<T>::type operator-(const Value& left, T right) {
				return static_cast<typename nearest_arithmetic_type<T>::type>(left) - right;
			}
			template<typename T> typename nearest_arithmetic_type<T>::type operator*(const Value& left, T right) {
				return static_cast<typename nearest_arithmetic_type<T>::type>(left) * right;
			}
			template<typename T> typename nearest_arithmetic_type<T>::type operator/(const Value& left, T right) {
				return static_cast<typename nearest_arithmetic_type<T>::type>(left) / right;
			}
			template<typename T> typename nearest_arithmetic_type<T>::type operator%(const Value& left, T right) {
				return static_cast<typename nearest_arithmetic_type<T>::type>(left) % right;
			}

			template<typename T> typename nearest_arithmetic_type<T>::type operator+(T left, const Value& right) {
				return left + static_cast<typename nearest_arithmetic_type<T>::type>(right);
			}
			template<typename T> typename nearest_arithmetic_type<T>::type operator-(T left, const Value& right) {
				return left - static_cast<typename nearest_arithmetic_type<T>::type>(right);
			}
			template<typename T> typename nearest_arithmetic_type<T>::type operator*(T left, const Value& right) {
				return left * static_cast<typename nearest_arithmetic_type<T>::type>(right);
			}
			template<typename T> typename nearest_arithmetic_type<T>::type operator/(T left, const Value& right) {
				return left / static_cast<typename nearest_arithmetic_type<T>::type>(right);
			}
			template<typename T> typename nearest_arithmetic_type<T>::type operator%(T left, const Value& right) {
				return left % static_cast<typename nearest_arithmetic_type<T>::type>(right);
			}

			bool operator!=(const Value& left, const char* right);
			bool operator!=(const Value&, Raw_string);
			bool operator!=(const Value&, const std::string&);

			/*
			 * Class Variable
			 * Handles ownership in scalar types. 
			 */
			template<typename T> class Variable : public T {
				public:
				typedef T Value;
				typedef Temp_template<Value> Temp;
				Variable(const Variable& other) : Value(other.interp, T::copy(other)) {
				}
				Variable(const Value& other) : Value(other.interp, T::copy(other)) {
				}
				Variable(const Temp& other) : Value(other.interp, (other.has_ownership() ? other.release() : T::copy(other))) {
				}
				template<typename U> Variable(const Temp_template<U>& other, typename boost::enable_if<typename boost::is_same<U, scalar::Value>::type, int>::type = 0) : Value(other.interp, T::copy(other)) { //TODO move?
				}
				protected:
				Variable(const scalar::Base& other, const override&) : Value(other.interp, T::copy(other)) {
				}
				template<typename U> Variable(const Temp_template<U>& other, const override&) : Value(other.interp, (other.has_ownership() ? other.release() : T::copy(other))) {
				}
				public:
				~Variable() {
					helper::decrement(*this);
				}
				void share() {
					helper::share(*this);
				}

				using T::operator=;
				const scalar::Temp_template<typename reference::Scalar_ref<T> > take_ref() const {
					return scalar::Temp_template<typename reference::Scalar_ref<T> >(T::interp, helper::take_ref(*this), true);
				}
			};

		}
	}

	/* Class Scalar
	 * Holds a Scalar variable. Can do anything a Scalar::Value can, with the difference that this is
	 * an owning datatype.
	 */
	class Scalar : public implementation::scalar::Variable<implementation::scalar::Value> {
		typedef implementation::scalar::Variable<implementation::scalar::Value> Parent;
		public:
		typedef implementation::scalar::Base Base;
		typedef implementation::scalar::Value Value;
		typedef implementation::scalar::Temp Temp;
		using Parent::operator=;

		Scalar(const Scalar&);
		Scalar(const Scalar::Base&);
		Scalar(const Scalar::Temp&);
		template<typename T> Scalar(const implementation::scalar::Temp_template<T>& other) : Parent(other, override()) {
		}

		static bool is_storage_type(const Any::Temp&);
	};

	std::ostream& operator<<(std::ostream& stream, const Scalar::Base&);

	const Scalar::Temp convert(const Scalar::Base& val);

	template<typename T> static inline const T convert(const Scalar::Temp& val) {
		return static_cast<T>(val);
	}
	template<> static inline const Raw_string convert<Raw_string>(const Scalar::Temp& val) {
		return val.operator Raw_string();
	}

	template<typename T> static inline const T convert(const Scalar::Base& val) {
		return convert<T>(convert(val));
	}

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
			public:
			
			Integer& operator=(const Integer&);
			Integer& operator=(IV);
			
			Integer& operator+=(IV);
			Integer& operator-=(IV);
			Integer& operator*=(IV);
			Integer& operator/=(IV);
			Integer& operator%=(IV);

			Integer& operator++();
			Integer operator++(int);
			Integer& operator--();
			Integer operator--(int);

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
			UV operator++(int);
			Uinteger& operator--();
			UV operator--(int);

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
			protected:
			String(interpreter*, SV*);
			public:

			operator const Raw_string() const;
			const char* get_raw() const;
//			operator const char*() const;
			unsigned length() const;
			operator bool() const;

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

	enum compared {
		LESS  = -1,
		EQUAL = 0,
		MORE  = 1
	};

	compared cmp(const Scalar::Base&, const Scalar::Base&);
	bool eq(const Scalar::Base&, const Scalar::Base&);

	namespace implementation {

		namespace reference {
			/*
			 * Meta function type_traits
			 * I might want to replace this with something else.
			 */
			template<typename T, typename Enable = void> struct type_traits;
			template<typename T> struct type_traits<T, typename boost::enable_if<typename boost::is_base_of<Scalar::Base, T>::type>::type> {
				typedef typename scalar::Temp_template<T> lvalue;
				typedef SV* raw_type;
			};
			template<> struct type_traits<Code> {
				typedef Code::Value lvalue;
				typedef CV* raw_type;
			};

			/* 
			 * Class Reference_base. All Ref<T>::Value's inherit from this.
			 */
			class Reference_base : public Scalar::Base, public implementation::method_calling< Reference_base > {
				protected:
				typedef Any::Temp Temp;
				Reference_base(interpreter*, SV*);
				public:
				bool is_object() const;
				bool isa(const char*) const;
				bool is_exactly(const char*) const;
				void weaken() const;
				void bless(const Package&);
				void bless(const char*);
				const char* get_classname() const;

				static SV* copy(const Scalar::Base&);
				static bool is_compatible_type(const scalar::Base& var);
			};

			template<typename T> class Ref_specialized : public Reference_base {
				protected:
				Ref_specialized(interpreter* _interp, SV* value) : Reference_base(_interp, value) {
				}
				public:
				typename type_traits<T>::lvalue operator*() const {
					const Any::Temp ret = helper::dereference(*static_cast<const typename ref<T>::type*>(this));
					if (!ref<T>::storage_type::is_storage_type(ret)) {
						throw Cast_exception(cast_error());
					}
					return typename type_traits<T>::lvalue(ret.interp, reinterpret_cast<typename type_traits<T>::raw_type>(ret.handle), false);
				}
				static const std::string& cast_error() {
					static const std::string message(T::Value::cast_error() + " reference");
					return message;
				}
				typename ref<T>::type& operator=(const typename ref<T>::type& other) {
					helper::set_scalar(*this, other);
					return static_cast<typename ref<T>::type*>(this);
				}
				typename ref<T>::type& operator=(const scalar::Temp& other) {
					if (! ref<T>::type::is_compatible_type(other)) {
						throw Cast_exception(cast_error);
					}
					helper::set_scalar(*this, other);
					return static_cast<typename ref<T>::type*>(this);
				}
			};

			/*
			 * Class Nonscalar<Any>. This reference can point to anything.
			 */
			template<> class Nonscalar<Any> : public Reference_base {
				public:
				Nonscalar(interpreter*, SV*);
				static const std::string& cast_error();
				Nonscalar<Any>& operator=(const Nonscalar<Any>&);
				Nonscalar<Any>& operator=(const Scalar::Temp&);
			};

			template<typename T> class Scalar_ref : public Ref_specialized<T> {
				protected:
				Scalar_ref(interpreter* _interp, SV* _handle) : Ref_specialized<T>(_interp, _handle) {
				}
				public:
				static bool is_compatible_type(const scalar::Base& var) {
					return Nonscalar<Any>::is_compatible_type(var) && is_that_type(helper::dereference(var));
				}
				static SV* copy(const scalar::Base& value) {
					if (!is_compatible_type(value)) {
						throw Cast_exception(Ref_specialized<T>::cast_error());
					}
					return scalar::Base::copy_sv(value);
				}
				
				private:
				static bool is_that_type(const Any::Temp& var) {
					return perl::Scalar::is_storage_type(var) 
						&& T::is_compatible_type(scalar::Temp(var.interp, reinterpret_cast<SV*>(var.handle), false));
				}
			};
			/*
			 * Metafunction ref.
			 * Chooses between scalar and nonscalar implementations.
			 */
			template<typename T, typename Enable> struct ref {
				typedef Nonscalar<T> type;
				typedef T storage_type;
			};
			template<typename T> struct ref<T, typename boost::enable_if<typename boost::is_base_of<scalar::Base, T>::type>::type > {
				typedef Scalar_ref<typename T::Value> type;
				typedef perl::Scalar storage_type;
			};

			/*
			 * Class Ref<Code>::Value
			 */
			template<> class Nonscalar<Code> : public Ref_specialized<Code>, public function_calling< Nonscalar<Code> > {
				protected:
				Nonscalar(interpreter*, SV*);

				static bool is_compatible_type(const scalar::Base&);
				static const std::string& cast_error();
			};
		}
	}

	/*
	 * Template class Ref<T>
	 * Handles a reference to a T. Exact capabilities depends on T.
	 */
	template<typename T = Any> class Ref : public implementation::scalar::Variable< typename implementation::reference::ref<T>::type> {
		typedef typename implementation::scalar::Variable< typename implementation::reference::ref<T>::type> Parent;
		public:
		Ref(const Ref& other) : Parent(other) {
		}
		Ref(const Scalar::Temp& val) : Parent(val) {
		}
		Ref(const typename Parent::Value& val) : Parent(val) {
		}
		Ref(const typename Parent::Temp& val) : Parent(val) {
		}
		using Parent::operator=;
	};
	template<> class Ref<Scalar> : public implementation::scalar::Variable<implementation::reference::ref<Scalar>::type> {
		typedef implementation::scalar::Variable< implementation::reference::ref<Scalar>::type> Parent;
		public:
		Ref(const Ref& other) : Parent(other) {
		}
		Ref(const Scalar::Temp& val) : Parent(val) {
		}
		Ref(const Parent::Value& val) : Parent(val) {
		}
		Ref(const Parent::Temp& val) : Parent(val) {
		}
		template<typename U> Ref(const implementation::reference::Scalar_ref<U>& other, typename boost::enable_if<typename boost::is_base_of<Scalar::Base, U>::type, int>::type foo = 0) : Parent(other, override()) { // accept scalar references if T is Any or Scalar, but postpone resolution until it is used.
		}
		using Parent::operator=;
	};
	template<> class Ref<Any> : public implementation::scalar::Variable<implementation::reference::ref<Any>::type> {
		typedef implementation::scalar::Variable< implementation::reference::ref<Any>::type> Parent;
		public:
		Ref(const Ref& other) : Parent(other) {
		}
		Ref(const Scalar::Temp& val) : Parent(val) {
		}
		Ref(const Parent::Value& val) : Parent(val) {
		}
		Ref(const Parent::Temp& val) : Parent(val) {
		}
		template<typename U> Ref(const implementation::reference::Nonscalar<U>& other) : Parent(other, override()) {
		}
		template<typename U> Ref(const implementation::reference::Scalar_ref<U>& other, typename boost::enable_if<typename boost::is_base_of<Scalar::Base, U>::type, int>::type foo = 0) : Parent(other, override()) { // accept scalar references if T is Any or Scalar, but postpone resolution until it is used.
		}
		using Parent::operator=;
	};
}
