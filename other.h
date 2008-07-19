namespace perl {
	//TODO: move these declarations to more appropriate places...
	class lock {
		interpreter* const interp;
		SV* const variable;
		SV* lock_SV(SV*);
		lock(const lock&);
		lock& operator=(const lock&);
		public:
		template<typename T> lock(const implementation::scalar::Variable<T>& _variable) : interp(_variable.interp), variable(lockSV(_variable.handle)) {
		}
		lock(const Array&);
		lock(const Hash&);
		~lock();
	};

	//TODO: Is this needed?
	class Scope {
		interpreter* interp;
		Scope(interpreter*);
		Scope(const Interpreter&);
//		Integer::value& 
	};
	
	namespace implementation {
		namespace reference {
			template<> struct type_traits<Array> {
				typedef Array::Temp lvalue;
				typedef AV* raw_type;
			};
			/*
			 * Class Ref<Array>::Value
			 */
			template<> class Nonscalar<Array> : public Ref_specialized<Array> {
				protected:
				Nonscalar(interpreter*, SV*);
				public:
				typedef Array::key_type key_type;
				Scalar::Temp operator[](key_type index) const;

				static bool is_compatible_type(const scalar::Base& var);
			};
			
			template<> struct type_traits<Hash> {
				typedef Hash::Temp lvalue;
				typedef HV* raw_type;
			};
			/*
			 * Class Ref<Hash>::Value
			 */
			template<> class Nonscalar<Hash> : public Ref_specialized<Hash> {
				protected:
				Nonscalar(interpreter*, SV*);
				public:
				Scalar::Temp operator[](Raw_string index) const;
				Scalar::Temp operator[](const scalar::Base& index) const;

				static bool is_compatible_type(const scalar::Base& var);
			};
		}
	}
}
