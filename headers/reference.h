namespace perl {
	namespace implementation {
		namespace reference {
			/*
			 * Meta function type_traits
			 * I might want to replace this with something else.
			 */
			template<typename T, typename = void> struct type_traits;
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
				~Reference_base() {
				}
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

			template<typename T> class Nonscalar_base : public Reference_base {
				protected:
				Nonscalar_base(interpreter* _interp, SV* value) : Reference_base(_interp, value) {
				}
				~Nonscalar_base() {
				}
				public:
				typename type_traits<T>::lvalue operator*() const {
					const Any::Temp ret = helper::dereference(*static_cast<const typename ref<T>::type*>(this));
					if (!T::is_storage_type(ret)) {
						throw Cast_exception(cast_error());
					}
					return typename type_traits<T>::lvalue(ret.interp, reinterpret_cast<typename type_traits<T>::raw_type>(ret.handle), false);
				}
				static const std::string& cast_error() {
					static const std::string message(T::Value::cast_error() + " reference");
					return message;
				}
				Nonscalar<T>& operator=(const typename ref<T>::type& other) {
					helper::set_scalar(*this, other);
					return *static_cast<Nonscalar<T>*>(this);
				}
				Nonscalar<T>& operator=(const scalar::Temp& other) {
					if (! Nonscalar<T>::is_compatible_type(other)) {
						throw Cast_exception(cast_error);
					}
					helper::set_scalar(*this, other);
					return *static_cast<Nonscalar<T>*>(this);
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

			template<typename T> class Scalar_ref : public Reference_base {
				protected:
				Scalar_ref(interpreter* _interp, SV* _handle) : Reference_base(_interp, _handle) {
				}
				public:
				static bool is_compatible_type(const scalar::Base& var) {
					return Nonscalar<Any>::is_compatible_type(var) && is_that_type(helper::dereference(var));
				}
				static SV* copy(const scalar::Base& value) {
					if (!is_compatible_type(value)) {
						throw Cast_exception(cast_error());
					}
					return scalar::Base::copy_sv(value);
				}
				
				scalar::Temp_template<T> operator*() const {
					if (!is_compatible_type(*this)) {
						throw Cast_exception(cast_error());
					}
					const Any::Temp ret = helper::dereference(*this); //TODO unoptimized
					return scalar::Temp_template<T>(ret.interp, reinterpret_cast<SV*>(ret.handle), false);
				}
				static const std::string& cast_error() {
					static const std::string message(T::Value::cast_error() + " reference");
					return message;
				}
				Scalar_ref<T>& operator=(const Scalar_ref& other) {
					helper::set_scalar(*this, other);
					return *this;
				}
				Scalar_ref<T>& operator=(const scalar::Temp& other) {
					if (! is_compatible_type(other)) {
						throw Cast_exception(cast_error);
					}
					helper::set_scalar(*this, other);
					return *this;
				}
				private:
				static bool is_that_type(const Any::Temp& var) {
					return helper::is_scalar_type(var) 
						&& T::is_compatible_type(scalar::Temp(var.interp, reinterpret_cast<SV*>(var.handle), false));
				}
			};
			/*
			 * Metafunction ref.
			 * Chooses between scalar and nonscalar implementations.
			 */
			template<typename T> struct ref<T, typename boost::disable_if<typename boost::is_base_of<scalar::Base, T>::type>::type> {
				typedef Nonscalar<T> type;
			};
			template<typename T> struct ref<T, typename boost::enable_if<typename boost::is_base_of<scalar::Base, T>::type>::type> {
				typedef Scalar_ref<typename T::Value> type;
			};

			/*
			 * Class Ref<Code>::Value
			 */
			template<> class Nonscalar<Code> : public Nonscalar_base<Code>, public function_calling< Nonscalar<Code> > {
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
	template<typename T = Any> class Ref : public implementation::scalar::Ownership< typename implementation::reference::ref<T>::type> {
		typedef typename implementation::scalar::Ownership< typename implementation::reference::ref<T>::type> Parent;
		public:
		Ref(const Ref& other) : Parent(other) {
		}
		Ref(const Scalar::Temp& val) : Parent(val, override()) {
		}
		Ref(const typename Parent::Value& val) : Parent(val) {
		}
		Ref(const typename Parent::Temp& val) : Parent(val) {
		}
		using Parent::operator=;
	};
	template<> class Ref<Scalar> : public implementation::scalar::Ownership<implementation::reference::ref<Scalar>::type> {
		typedef implementation::scalar::Ownership< implementation::reference::ref<Scalar>::type> Parent;
		public:
		Ref(const Ref& other) : Parent(other) {
		}
		Ref(const Scalar::Temp& val) : Parent(val, override()) {
		}
		Ref(const Parent::Value& val) : Parent(val) {
		}
		Ref(const Parent::Temp& val) : Parent(val) {
		}
		template<typename U> Ref(const implementation::reference::Scalar_ref<U>& other, typename boost::enable_if<typename boost::is_base_of<Scalar::Base, U>::type, int>::type foo = 0) : Parent(other, override()) { // accept scalar references if T is Any or Scalar, but postpone resolution until it is used.
		}
		using Parent::operator=;
	};
	template<> class Ref<Any> : public implementation::scalar::Ownership<implementation::reference::ref<Any>::type> {
		typedef implementation::scalar::Ownership< implementation::reference::ref<Any>::type> Parent;
		public:
		Ref(const Ref& other) : Parent(other) {
		}
		Ref(const Scalar::Temp& val) : Parent(val, override()) {
		}
		Ref(const Parent::Value& val) : Parent(val) {
		}
		Ref(const Parent::Temp& val) : Parent(val) {
		}
		template<typename U> Ref(const implementation::reference::Nonscalar<U>& other) : Parent(other, override()) {
		}
		template<typename U> Ref(const implementation::reference::Scalar_ref<U>& other, typename boost::enable_if<typename boost::is_base_of<Scalar::Base, U>::type, int>::type = 0) : Parent(other, override()) { // accept scalar references if T is Any or Scalar, but postpone resolution until it is used.
		}
		using Parent::operator=;
	};
}
