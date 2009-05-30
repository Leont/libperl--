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

	namespace implementation {
		namespace scalar {
			class Value;
			template<typename T> class Variable;
		}
	}	
	typedef implementation::scalar::Variable<implementation::scalar::Value> Scalar;

	namespace implementation {
		bool is_this_type(const Any::Temp& var, unsigned int type);

		namespace reference {
			template<typename T> class Nonscalar;
			template<typename T> class Scalar_ref;
			class Reference_base;
		}

		namespace scalar {
			class Value;
			template<typename T> class Temp_template;
			typedef Temp_template<Value> Temp;

			template<typename T> class Referencable {
				public:
				const scalar::Temp_template<typename reference::Scalar_ref<T> > take_ref() const;
			};
			/*
			 * Class Scalar::Base
			 * This class represents scalar values. It's a thin interface, as different kinds of scalar
			 * values have little in common.
			 */
			class Base : public Referencable<Base> {
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

				protected:
				static SV* copy_sv(const Base&);
				~Base() {
				}
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
			bool is_scalar_type(const Any::Temp&);
		}

		namespace scalar {

			template<typename T> const scalar::Temp_template<typename reference::Scalar_ref<T> > Referencable<T>::take_ref() const {
				return Temp_template<reference::Scalar_ref<T> >(static_cast<const T*>(this)->interp, helper::take_ref(*static_cast<const T*>(this)), true);
			}
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
				template<typename U> Temp_template(const Temp_template<U>& other, typename boost::enable_if<typename accept_anything<U>::type, int>::type = 1) : Base_type(other.interp, other.get_SV(false)), owns(other.owns), transferable(other.transferable) {
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
		template<typename T, typename = void, typename = void> struct nearest_arithmetic_type {
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
			~Perl_stack() {
			}
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
			int match(REGEXP*, SV*, IV, IV);
			public:
			explicit Call_stack(interpreter*);

			const scalar::Temp method_scalar(const char* name);
			const array::Temp method_list(const char* name);
			const scalar::Temp sub_scalar(const char* name);
			const scalar::Temp sub_scalar(const implementation::reference::Nonscalar<Code>& ref);
			const scalar::Temp sub_scalar(const scalar::Value& ref);
			const array::Temp sub_list(const char* name);
			const array::Temp sub_list(const implementation::reference::Nonscalar<Code>& ref);
			const array::Temp sub_list(const scalar::Value& ref);

			const scalar::Temp_template<implementation::String> pack(const Raw_string pattern);
			const array::Temp unpack(const Raw_string pattern, const Raw_string value);

			const scalar::Temp eval_scalar(SV*);
			const array::Temp eval_list(SV*);

			int match_scalar(REGEXP*, const scalar::Base&, IV flags);
			const array::Temp match_array(REGEXP*, const scalar::Base&, IV flags);

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
			protected:
			~function_calling() {
			}
			public:
			const scalar_type operator()() const {
				return implementation::Call_stack(self().interp).sub_scalar(self());
			}
			template<typename T1> scalar_type operator()(const T1& t1) const {
				return implementation::Call_stack(self().interp).push(t1).sub_scalar(self());
			}
			template<typename T1, typename T2> scalar_type operator()(const T1& t1, const T2& t2) const {
				return implementation::Call_stack(self().interp).push(t1, t2).sub_scalar(self());
			}
			template<typename T1, typename T2, typename T3> scalar_type operator()(const T1& t1, const T2& t2, const T3& t3) const {
				return implementation::Call_stack(self().interp).push(t1, t2, t3).sub_scalar(self());
			}
			template<typename T1, typename T2, typename T3, typename T4> scalar_type operator()(const T1& t1, const T2& t2, const T3& t3, const T4& t4) const {
				return implementation::Call_stack(self().interp).push(t1, t2, t3, t4).sub_scalar(self());
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5> scalar_type operator()(const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5) const {
				return implementation::Call_stack(self().interp).push(t1, t2, t3, t4, t5).sub_scalar(self());
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6> scalar_type operator()(const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6) const {
				return implementation::Call_stack(self().interp).push(t1, t2, t3, t4, t5, t6).sub_scalar(self());
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7> scalar_type operator()(const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7) const {
				return implementation::Call_stack(self().interp).push(t1, t2, t3, t4, t5, t6, t7).sub_scalar(self());
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8> scalar_type operator()(const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7, const T8& t8) const {
				return implementation::Call_stack(self().interp).push(t1, t2, t3, t4, t5, t6, t7, t8).sub_scalar(self());
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9> scalar_type operator()(const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7, const T8& t8, const T9& t9) const {
				return implementation::Call_stack(self().interp).push(t1, t2, t3, t4, t5, t6, t7, t8, t9).sub_scalar(self());
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10> scalar_type operator()(const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7, const T8& t8, const T9& t9, const T10& t10) const {
				return implementation::Call_stack(self().interp).push(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10).sub_scalar(self());
			}


			array_type list() const {
				return implementation::Call_stack(self().interp).sub_list(self());
			}
			template<typename T1> array_type list(const T1& t1) const {
				return implementation::Call_stack(self().interp).push(t1).sub_list(self());
			}
			template<typename T1, typename T2> array_type list(const T1& t1, const T2& t2) const {
				return implementation::Call_stack(self().interp).push(t1, t2).sub_list(self());
			}
			template<typename T1, typename T2, typename T3> array_type list(const T1& t1, const T2& t2, const T3& t3) const {
				return implementation::Call_stack(self().interp).push(t1, t2, t3).sub_list(self());
			}
			template<typename T1, typename T2, typename T3, typename T4> array_type list(const T1& t1, const T2& t2, const T3& t3, const T4& t4) const {
				return implementation::Call_stack(self().interp).push(t1, t2, t3, t4).sub_list(self());
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5> array_type list(const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5) const {
				return implementation::Call_stack(self().interp).push(t1, t2, t3, t4, t5).sub_list(self());
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6> array_type list(const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6) const {
				return implementation::Call_stack(self().interp).push(t1, t2, t3, t4, t5, t6).sub_list(self());
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7> array_type list(const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7) const {
				return implementation::Call_stack(self().interp).push(t1, t2, t3, t4, t5, t6, t7).sub_list(self());
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8> array_type list(const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7, const T8& t8) const {
				return implementation::Call_stack(self().interp).push(t1, t2, t3, t4, t5, t6, t7, t8).sub_list(self());
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9> array_type list(const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7, const T8& t8, const T9& t9) const {
				return implementation::Call_stack(self().interp).push(t1, t2, t3, t4, t5, t6, t7, t8, t9).sub_list(self());
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10> array_type list(const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7, const T8& t8, const T9& t9, const T10& t10) const {
				return implementation::Call_stack(self().interp).push(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10).sub_list(self());
			}
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
			protected:
			~method_calling() {
			}
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

			array_type call_list(const char* name) const {
				return implementation::Call_stack(self().interp).push(self()).method_list(name);
			}
			template<typename T1> array_type call_list(const char* name, const T1& t1) const {
				return implementation::Call_stack(self().interp).push(self(), t1).method_list(name);
			}
			template<typename T1, typename T2> array_type call_list(const char* name, const T1& t1, const T2& t2) const {
				return implementation::Call_stack(self().interp).push(self(), t1, t2).method_list(name);
			}
			template<typename T1, typename T2, typename T3> array_type call_list(const char* name, const T1& t1, const T2& t2, const T3& t3) const {
				return implementation::Call_stack(self().interp).push(self(), t1, t2, t3).method_list(name);
			}
			template<typename T1, typename T2, typename T3, typename T4> array_type call_list(const char* name, const T1& t1, const T2& t2, const T3& t3, const T4& t4) const {
				return implementation::Call_stack(self().interp).push(self(), t1, t2, t3, t4).method_list(name);
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5> array_type call_list(const char* name, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5) const {
				return implementation::Call_stack(self().interp).push(self(), t1, t2, t3, t4, t5).method_list(name);
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6> array_type call_list(const char* name, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6) const {
				return implementation::Call_stack(self().interp).push(self(), t1, t2, t3, t4, t5, t6).method_list(name);
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7> array_type call_list(const char* name, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7) const {
				return implementation::Call_stack(self().interp).push(self(), t1, t2, t3, t4, t5, t6, t7).method_list(name);
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8> array_type call_list(const char* name, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7, const T8& t8) const {
				return implementation::Call_stack(self().interp).push(self(), t1, t2, t3, t4, t5, t6, t7, t8).method_list(name);
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9> array_type call_list(const char* name, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7, const T8& t8, const T9& t9) const {
				return implementation::Call_stack(self().interp).push(self(), t1, t2, t3, t4, t5, t6, t7, t8, t9).method_list(name);
			}
			template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10> array_type call_list(const char* name, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7, const T8& t8, const T9& t9, const T10& t10) const {
				return implementation::Call_stack(self().interp).push(self(), t1, t2, t3, t4, t5, t6, t7, t8, t9, t10).method_list(name);
			}
		};

		namespace hash {
			class String_callback;
			class Scalar_callback;
		}

		class String;

		namespace scalar {

			/*
			 * Class Scalar::Value
			 * This class can be converted into anything any scalar can be converted in. If the the convertion 
			 * fails, it throws an exception. It can also be used as a reference, but the same caveat applies.
			 */
			class Value: public Base, public implementation::function_calling<Value>, public implementation::method_calling<Value>, public Referencable<Value> {
				protected:
				Value(interpreter*, SV*);
				~Value() {
				}
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
				operator const std::string() const;
//				operator const char*() const;
				operator bool() const;
				bool as_bool() const;

				bool defined() const;
				size_t length() const;
				const array::Temp unpack(const Raw_string) const;
			
				scalar::Temp operator[](int) const;
				scalar::Temp operator[](Raw_string index) const;
				scalar::Temp operator[](const Base& index) const;

				bool is_object() const;
				bool isa(const char*) const;
				bool is_exactly(const char*) const;
				const char* get_classname() const;

				static SV* copy(const Base&);
				using Referencable<Value>::take_ref;
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
			template<typename T> class Ownership  : public T {
				public:
				typedef T Value;
				typedef Temp_template<Value> Temp;
				protected:
				Ownership(const Ownership& other) : Value(other.interp, T::copy(other)) {
				}
				Ownership(const Value& other) : Value(other.interp, T::copy(other)) {
				}
				Ownership(const Temp& other) : Value(other.interp, (other.has_ownership() ? other.release() : T::copy(other))) {
				}

				Ownership(const scalar::Base& other, const override&) : Value(other.interp, T::copy(other)) {
				}
				template<typename U> Ownership(const Temp_template<U>& other, const override&) : Value(other.interp, (other.has_ownership() ? other.release() : T::copy(other))) {
				}
				~Ownership() {
					helper::decrement(*this);
				}
				public:
				void share() {
					helper::share(*this);
				}

				using T::operator=;
			};
			template<typename T> class Variable : public Ownership<T> {
				public:
				using Ownership<T>::operator=;
				Variable(const Variable& other) : Ownership<T>(other) {
				}
				Variable(const typename Ownership<T>::Value& other) : Ownership<T>(other) {
				}
				Variable(const typename Ownership<T>::Temp& other) : Ownership<T>(other) {
				}
				Variable(const scalar::Temp& other) : Ownership<T>(other, override()) {
				}
			};
			/* Class Scalar
			 * Holds a Scalar variable. Can do anything a Scalar::Value can, with the difference that this is
			 * an owning datatype.
			 */
			template<> class Variable<Value> : public Ownership<Value> {
				public:
				typedef Base Base;
				using Ownership<Value>::operator=;

				Variable(const Variable& other) : implementation::scalar::Ownership<implementation::scalar::Value>(other) {
				}
				Variable(const Base& other) : implementation::scalar::Ownership<implementation::scalar::Value>(other, override()) {
				}
				Variable(const Temp& other) : implementation::scalar::Ownership<implementation::scalar::Value>(other) {
				}
				template<typename T> Variable(const Temp_template<T>& other) : implementation::scalar::Ownership<implementation::scalar::Value>(other, override()) {
				}
			};
		}
	}

	std::ostream& operator<<(std::ostream& stream, const Scalar::Base&);

	const Scalar::Temp convert(const Scalar::Base& val);

	namespace {
		template<typename T> inline const T convert(const Scalar::Temp& val) {
			return static_cast<T>(val);
		}
		template<> inline const Raw_string convert<Raw_string>(const Scalar::Temp& val) {
			return val.operator Raw_string();
		}

		template<typename T> inline const T convert(const Scalar::Base& val) {
			return convert<T>(perl::convert(val));
		}
	}

	enum compared {
		LESS  = -1,
		EQUAL = 0,
		MORE  = 1
	};

	compared cmp(const Scalar::Base&, const Scalar::Base&);
	bool eq(const Scalar::Base&, const Scalar::Base&);
}
