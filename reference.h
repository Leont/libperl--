namespace perl {
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
