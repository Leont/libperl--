namespace perl {
	namespace implementation {	
		/*
		 * Magic subroutine functions
		 */
		const Raw_string get_magic_string(interpreter*, SV*);
		void set_magic_string(interpreter*, SV*, Raw_string);
		void set_magic_string(interpreter*, SV*, const void*, unsigned length);
		template<typename T> void set_magic_object(interpreter* interp, SV* handle, const T& object) {
			set_magic_string(interp, handle, &object, sizeof object);
		}

		void* get_magic_ptr(interpreter* interp, SV* var, int);
		void** get_magic_object_impl(interpreter*, SV*, int);
		template<typename T> T* get_magic_object(const Scalar::Base& var) {
			return reinterpret_cast<T*>(*get_magic_object_impl(var.interp, var.get_SV(false), sizeof(T*)));
		}
		template<typename T> T& get_magic_buffer(interpreter* interp, SV* var) {
			return *reinterpret_cast<T*>(get_magic_object_impl(interp, var, sizeof(T)));
		}
		template<typename T> T& get_magic_buffer(const Scalar::Base& var) {
			return get_magic_buffer<T>(var.interp, var.get_SV(false));
		}

		bool has_magic_string(interpreter*, SV*);
//		bool has_magic_string(Scalar::Base&);

		typedef int (*magic_fun)(interpreter*, SV*, MAGIC*);
		void attach_getset_magic(interpreter* interp, SV* var, magic_fun get_val, magic_fun set_val, const void* buffer, size_t buffer_length);

		void* get_magic_ptr(const MAGIC*);
		template<typename T> T* get_magic_ptr(const MAGIC* magic) {
			return reinterpret_cast<T*>(get_magic_ptr(magic));
		}

		struct Object_buffer {
			void* ref;
			bool owns;
			Object_buffer(void* _ref, bool _owns) : ref(_ref), owns(_owns) {
			}
			template<typename T> T*& get() {
				return reinterpret_cast<T*&>(ref);
			}
		};

		namespace classes {
			struct State {
				const char* classname;
				MGVTBL* magic_table;
				bool has_destructor;
				bool hash;
				bool persistent;
				State(const char* _classname, MGVTBL* _magic_table) : classname(_classname), magic_table(_magic_table) {
				}
				private:
				State(const State&);
			};
		}
		SV* make_magic_object(interpreter*, void*, const classes::State&);

		namespace magic {
			struct read_type {};
			struct write_type {};
			template<typename T, typename U, typename V = U> class Wrapper {
				typedef void (T::*read_method_type)(U&);
				typedef void (T::*write_method_type)(V&);
				T& object;
				const read_method_type reader;
				const write_method_type writer;
				void read(U& arg) const {
					(object.*reader)(arg);
				}
				void write(V& arg) const {
					(object.*writer)(arg);
				}
				public:
				static int read(interpreter* interp, SV* var, MAGIC* magic_ptr) {
					const Wrapper& tmp = *implementation::get_magic_ptr<Wrapper>(magic_ptr);
					Scalar::Temp val(interp, var, false);
					tmp.read(val);
					return 0;
				}
				static int write(interpreter* interp, SV* var, MAGIC* magic_ptr) {
					const Wrapper& tmp = *implementation::get_magic_ptr<Wrapper>(magic_ptr);
					Scalar::Temp val(interp, var, false);
					tmp.write(val);
					return 0;
				}
				Wrapper(T& _object, read_method_type method, const read_type&) : object(_object), reader(method), writer(NULL) {
				}
				Wrapper(T& _object, write_method_type method, const write_type&) : object(_object), reader(NULL), writer(method) {
				}
				Wrapper(T& _object, read_method_type _reader, write_method_type _writer) : object(_object), reader(_reader), writer(_writer) {
				}
			};
			template<typename T> int var_read(interpreter* interp, SV* var, MAGIC* magic_ptr) {
				const T& tmp = *implementation::get_magic_ptr<T>(magic_ptr);
				Scalar::Temp val(interp, var, false);
				if (val != tmp) {
					val = tmp;
				}
				return 0;
			}
			template<typename T> int var_write(interpreter* interp, SV* var, MAGIC* magic_ptr) {
				T& tmp = *implementation::get_magic_ptr<T>(magic_ptr);
				Scalar::Temp val(interp, var, false);
				tmp = val;
				return 0;
			}
		}
	}

	namespace magical {
		using namespace implementation::magic;
		template<typename T, typename U> static void readonly(const Scalar::Base& var, T& object, void (T::*get_value)(U&)) {
			const Wrapper<T, U> funcs(object, get_value, read_type());
			attach_getset_magic(var.interp, var.get_SV(false), Wrapper<T, U>::read, NULL, &funcs, sizeof(funcs));
		}
		template<typename T, typename U> static void writeonly(const Scalar::Base& var, T& object, void (T::*set_value)(U&)) {
			const Wrapper<T, U> funcs(object, set_value, write_type());
			attach_getset_magic(var.interp, var.get_SV(false), NULL, Wrapper<T, U>::write, &funcs, sizeof(funcs));
		}
		template<typename T, typename U, typename V> static void readwrite(const Scalar::Base& var, T& object, void (T::*get_value)(U&), void (T::*set_value)(V&)) {
			const Wrapper<T, U, V> funcs(object, get_value, set_value);
			attach_getset_magic(var.interp, var.get_SV(false), Wrapper<T, U, V>::read, Wrapper<T, U, V>::write, &funcs, sizeof(funcs));
		}

		template<typename T> static void readonly(const Scalar::Base& var, T& object) {
			attach_getset_magic(var.interp, var.get_SV(false), var_read<T>, NULL, &object, 0);
		}
		template<typename T> static void writeonly(const Scalar::Base& var, T& object) {
			attach_getset_magic(var.interp, var.get_SV(false), NULL, var_write<T>, &object, 0);
		}
		template<typename T> static void readwrite(const Scalar::Base& var, T& object) {
			attach_getset_magic(var.interp, var.get_SV(false), var_read<T>, var_write<T>, &object, 0);
		}
	};

	namespace implementation {
		/*
		 * C++ to perl exporting stuff.
		 * Here be dragons!
		 */
		struct Perl_mark {
			Perl_mark(int, SV**, unsigned);
			const int ax;
			SV* const * const mark;
			const unsigned items;
		};
	}

	class Argument_stack : public implementation::Perl_stack {
		implementation::Perl_mark marker;
		unsigned return_num;
		public:
		explicit Argument_stack(interpreter*);
		const Scalar::Temp operator[](unsigned) const;
		Scalar::Temp operator[](unsigned);
		void pre_push();
		template <typename T> void returns(const T& t) {
			pre_push();
			Perl_stack::push(t);
			return_num++;
		}
		~Argument_stack();
		const Array::Temp get_arg() const;
		Array::Temp get_arg();
		const Scalar::Temp call(const char* name);
		const Scalar::Temp call(const Ref<Code>::Value& name);
		context get_contest() const;
	};

	namespace implementation {
		const Code::Value export_as(interpreter*, const char* name, void (*)(interpreter*, CV*), const void*, int);
		template<typename T> const Code::Value export_as(interpreter* interp, const char* name, void(*glue)(interpreter*, CV*), const T& func) {
			return export_as(interp, name, glue, &func, sizeof func);
		}

		template<typename T> T& get_function_pointer(interpreter* interp, CV* cef) {
			Raw_string ret = get_magic_string(interp, reinterpret_cast<SV*>(cef));
			if (ret.length < sizeof(T)) {
				throw Runtime_exception("Magical error!");
			}
			return *reinterpret_cast<T*>(const_cast<char*>(ret.value));
		}

		void die(interpreter*, const char* message);
#define typemap_cast static_cast

		//TODO: make it throw an object
#define TRY_OR_THROW(a) try {\
		a;\
		}\
		catch(std::exception& e) {\
			die(me_perl, e.what());\
		}\
		catch(...) {\
			std::terminate();\
		}

		//Section functions

		template<typename R, typename A> struct export_flat {
			typedef R (*func_ptr)(A);
			static void subroutine(interpreter* me_perl, CV* cef) {
				Argument_stack stack(me_perl);
				const func_ptr ref = implementation::get_function_pointer<func_ptr>(me_perl, cef);
				TRY_OR_THROW(stack.returns(ref(stack)));
			}
		};
		template<typename R>struct export_sub_0 {
			typedef R (*func_ptr)();
			static void subroutine(interpreter* me_perl, CV* cef) {
				Argument_stack arg_stack(me_perl);
				const func_ptr ref = get_function_pointer<func_ptr>(me_perl, cef);
				TRY_OR_THROW(arg_stack.returns(ref()));
			}
		};
		template<typename R, typename A1> struct export_sub_1 {
			typedef R (*func_ptr)(A1);
			static void subroutine(interpreter* me_perl, CV* cef) {
				Argument_stack arg_stack(me_perl);
				const func_ptr ref = implementation::get_function_pointer<func_ptr>(me_perl, cef);
				TRY_OR_THROW(arg_stack.returns(ref(typemap_cast<A1>(arg_stack[0]))));
			}
		};
		template<typename R, typename A1, typename A2> struct export_sub_2 {
			typedef R (*func_ptr)(A1, A2);
			static void subroutine(interpreter* me_perl, CV* cef) {
				Argument_stack arg_stack(me_perl);
				const func_ptr ref = implementation::get_function_pointer<func_ptr>(me_perl, cef);
				TRY_OR_THROW(arg_stack.returns(ref(typemap_cast<A1>(arg_stack[0]), typemap_cast<A2>(arg_stack[1]))));
			}
		};
		template<typename R, typename A1, typename A2, typename A3> struct export_sub_3 {
			typedef R (*func_ptr)(A1, A2);
			static void subroutine(interpreter* me_perl, CV* cef) {
				Argument_stack arg_stack(me_perl);
				const func_ptr ref = implementation::get_function_pointer<func_ptr>(me_perl, cef);
				TRY_OR_THROW(arg_stack(ref(typemap_cast<A1>(arg_stack[0]), typemap_cast<A2>(arg_stack[1]), typemap_cast<A3>(arg_stack[2]))));
			}
		};

		template<typename T> struct export_vflat {
			typedef void (*func_ptr)(T&);
			static void subroutine(interpreter* me_perl, CV* cef) {
				Argument_stack stack(me_perl);
				const func_ptr ref = implementation::get_function_pointer<func_ptr>(me_perl, cef);
				ref(stack);
			}
		};
		struct export_sub_v0 {
			typedef void(*func_ptr)();
			static void subroutine(interpreter* me_perl, CV* cef) {
				const func_ptr ref = implementation::get_function_pointer<func_ptr>(me_perl, cef);
				ref();
			}
		};
		template<typename A1> struct export_sub_v1 {
			typedef void(*func_ptr)(A1);
			static void subroutine(interpreter* me_perl, CV* cef) {
				Argument_stack arg_stack(me_perl);
				const func_ptr ref = implementation::get_function_pointer<func_ptr>(me_perl, cef);
				ref(typemap_cast<A1>(arg_stack[0]));
			}
		};
		template<typename A1, typename A2> struct export_sub_v2 {
			typedef void(*func_ptr)(A1, A2);
			static void subroutine(interpreter* me_perl, CV* cef) {
				Argument_stack arg_stack(me_perl);
				const func_ptr ref = implementation::get_function_pointer<func_ptr>(me_perl, cef);
				ref(typemap_cast<A1>(arg_stack[0], arg_stack[1]));
			}
		};

		template<typename R, typename A1> const Code::Value export_flatsub(interpreter* interp, const char* name, R (fptr)(A1)) {
			return export_as(interp, name, export_flat<R, A1>::subroutine, reinterpret_cast<void*>(fptr));
		}
		template<typename A> const Code::Value export_flatsub(interpreter* interp, const char* name, void (fptr)(A)) {
			return export_as(interp, name, export_vflat<A>::subroutine, fptr);
		}

		template<typename R> const Code::Value export_sub(interpreter* interp, const char* name, R (fptr)()) {
			return export_as(interp, name, export_sub_0<R>::subroutine, reinterpret_cast<void*>(fptr));
		}
		static inline const Code::Value export_sub(interpreter* interp, const char* name, void (fptr)()) {
			return export_as(interp, name, export_sub_v0::subroutine, fptr);
		}

		template<typename R, typename A1> const Code::Value export_sub(interpreter* interp, const char* name, R (fptr)(A1)) {
			return export_as(interp, name, export_sub_1<R, A1>::subroutine, reinterpret_cast<void*>(fptr));
		}
		template<typename A1> const Code::Value export_sub(interpreter* interp, const char* name, void (fptr)(A1)) {
			return export_as(interp, name, export_sub_v1<A1>::subroutine, reinterpret_cast<void*>(fptr));
		}

		template<typename R, typename A1, typename A2> const Code::Value export_sub(interpreter* interp, const char* name, R (fptr)(A1, A2)) {
			return export_as(interp, name, export_sub_2<R, A1, A2>::subroutine, reinterpret_cast<void*>(fptr));
		}
		template<typename A1, typename A2> const Code::Value export_sub(interpreter* interp, const char* name, void (fptr)(A1, A2)) {
			return export_as(interp, name, export_sub_v2<A1, A2>::subroutine, reinterpret_cast<void*>(fptr));
		}

		template<typename R, typename A1, typename A2, typename A3> const Code::Value export_sub(interpreter* interp, const char* name, R (fptr)(A1, A2, A3)) {
			return export_as(interp, name, export_sub_3<R, A1, A2, A3>::subroutine, reinterpret_cast<void*>(fptr));
		}

		//Section methods
		
		template<typename R, typename T> struct export_method_0 {
			typedef R (T::*func_ptr)();
			static void method(interpreter* me_perl, CV* cef) {
				Argument_stack arg_stack(me_perl);
				const func_ptr ref = implementation::get_function_pointer<func_ptr>(me_perl, cef);
				TRY_OR_THROW(arg_stack.returns((get_magic_object<T>(arg_stack[0])->*ref)()));
			}
		};

		template<typename T> struct export_method_v0 {
			typedef void (T::*func_ptr)();
			static void method(interpreter* me_perl, CV* cef) {
				Argument_stack arg_stack(me_perl);
				const func_ptr ref = implementation::get_function_pointer<func_ptr>(me_perl, cef);
				TRY_OR_THROW((get_magic_object<T>(arg_stack[0])->*ref)());
			}
		};

		template<typename R, typename T, typename A1> struct export_method_1 {
			typedef R (T::*func_ptr)(A1);
			static void method(interpreter* me_perl, CV* cef) {
				Argument_stack arg_stack(me_perl);
				const func_ptr ref = implementation::get_function_pointer<func_ptr>(me_perl, cef);
				TRY_OR_THROW(arg_stack.returns((get_magic_object<T>(arg_stack[0])->*ref)(static_cast<A1>(arg_stack[1]))));
			}
		};

		template<typename T, typename A1> struct export_method_v1 {
			typedef void (T::*func_ptr)(A1);
			static void method(interpreter* me_perl, CV* cef) {
				Argument_stack arg_stack(me_perl);
				const func_ptr ref = implementation::get_function_pointer<func_ptr>(me_perl, cef);
				TRY_OR_THROW((get_magic_object<T>(arg_stack[0])->*ref)(static_cast<A1>(arg_stack[1])));
			}
		};

		template<typename T, typename R> static void export_method(interpreter* const interp, const char* name, R (T::* const fptr)()) {
			implementation::export_as(interp, name, export_method_0<R,T>::method, fptr);
		}
		template<typename T> static void export_method(interpreter* const interp, const char* name, void (T::* const fptr)()) {
			implementation::export_as(interp, name, export_method_v0<T>::method, fptr);
		}
		template<typename T, typename R, typename A1> static void export_method(interpreter* const interp, const char* name, R (T::*fptr)(A1)) {
			implementation::export_as(interp, name, export_method_1<R, T, A1>::method, fptr);
		}
		template<typename T, typename A1> static void export_method(interpreter* const interp, const char* name, void (T::* const fptr)(A1)) {
			implementation::export_as(interp, name, export_method_v1<T, A1>::method, fptr);
		}

		//Section constructors

		struct constructor_info {
			void* fptr;
			classes::State& class_state;
			constructor_info(void* _fptr, classes::State& _state) : fptr(_fptr), class_state(_state) {
			}
			template<typename T> constructor_info(T* _fptr, classes::State& _state, bool _persistent = false) : fptr(reinterpret_cast<void*>(_fptr)), class_state(_state) {
			}
			template<typename T> T get() const {
				return reinterpret_cast<T>(fptr);
			}
		};
		template<typename T> struct constructor_exporter {
			typedef T return_type;
			struct arg0 {
				typedef T* (*func_ptr)();
				static void subroutine(interpreter* me_perl, CV* cef) {
					Argument_stack arg_stack(me_perl);
					const constructor_info data = implementation::get_function_pointer<constructor_info>(me_perl, cef);
					const func_ptr ref = data.get<func_ptr>();
					TRY_OR_THROW(arg_stack.returns(store_in_cache(me_perl, ref(), data.class_state)));
				}
			};
			template<typename A1> struct arg1 {
				typedef T* (*func_ptr)(A1);
				static void subroutine(interpreter* me_perl, CV* cef) {
					Argument_stack arg_stack(me_perl);
					const constructor_info data = implementation::get_function_pointer<constructor_info>(me_perl, cef);
					const func_ptr ref = data.get<func_ptr>();
					TRY_OR_THROW(arg_stack.returns(store_in_cache(me_perl, ref(static_cast<A1>(arg_stack[1])), data.class_state)));
				}
			};
			template<typename A1, typename A2> struct arg2 {
				typedef T (*func_ptr)(A1, A2);
				static void subroutine(interpreter* me_perl, CV* cef) {
					Argument_stack arg_stack(me_perl);
					const constructor_info data = implementation::get_function_pointer<constructor_info>(me_perl, cef);
					const func_ptr ref = data.get<func_ptr>();
					TRY_OR_THROW(arg_stack.returns(store_in_cache(me_perl, ref(static_cast<A1>(arg_stack[1]), static_cast<A2>(arg_stack[2])), data.class_state)));
				}
			};
			template<typename A1, typename A2, typename A3> struct arg3 {
				typedef T (*func_ptr)(A1, A2);
				static void subroutine(interpreter* me_perl, CV* cef) {
					Argument_stack arg_stack(me_perl);
					const constructor_info data = implementation::get_function_pointer<constructor_info>(me_perl, cef);
					const func_ptr ref = data.get<func_ptr>();
					TRY_OR_THROW(arg_stack.returns(store_in_cache(me_perl, ref(static_cast<A1>(arg_stack[1]), static_cast<A2>(arg_stack[2]), static_cast<A3>(arg_stack[3])), data.class_state)));
				}
			};
			public:
			static const Code::Value export_cons(interpreter* interp, const char* name, T* (fptr)(), classes::State& state) {
				return export_as(interp, name, arg0::subroutine, constructor_info(fptr, state));
			}
			template<typename A1> static const Code::Value export_cons(interpreter* interp, const char* name, T* (*fptr)(A1), classes::State& state) {
				return export_as(interp, name, arg1<A1>::subroutine, constructor_info(fptr, state));
			}
			template<typename A1, typename A2> static const Code::Value export_cons(interpreter* interp, const char* name, T* (fptr)(A1, A2), classes::State& state) {
				return export_as(interp, name, arg2<A1, A2>::subroutine, constructor_info(fptr, state));
			}
			template<typename A1, typename A2, typename A3> static const Code::Value export_cons(interpreter* interp, const char* name, T* (fptr)(A1, A2, A3), classes::State& state) {
				return export_as(interp, name, arg3<A1, A2, A3>::subroutine, constructor_info(fptr, state));
			}
		};
#undef TRY_OR_THROW

		Ref<Any>::Temp get_from_cache(interpreter*, const void*);
		Ref<Any>::Temp store_in_cache(interpreter*, void*, const implementation::classes::State&);
	 }

	template<typename T> class Class;

	class Package : public implementation::method_calling<Package> {
		interpreter* const interp;
		const std::string package_name;
		HV* const stash;
		Package& operator=(const Package&);
		friend class implementation::method_calling<Package>;
		friend class implementation::reference::Reference_base;
		friend class implementation::Stash;
		template<typename T> friend class Class;
		public:
		Package(const Package&);
		Package(const Interpreter&, const char*, bool = false);
		Package(interpreter*, const char*, bool = false);
		Package(interpreter*, SV*, bool = false);
		const std::string& get_name() const;
		operator const std::string&() const;
		
		template<typename T> Code::Value export_sub(const char* name, const T& func) {
			return implementation::export_sub(interp, (package_name + "::" + name).c_str(), func);
		}
		template<typename T> Code::Value export_flatsub(const char* name, const T& func) {
			return implementation::export_flatsub(interp, (package_name + "::" + name).c_str(), func);
		}

		private:
		template<typename T> void export_method(const char* name, const T& func) {
			implementation::export_method(interp, (package_name + "::" + name).c_str(), func);
		}
		template<typename T, typename U> void export_constructor(const char* name, const U& constructor, implementation::classes::State& state) {
			implementation::constructor_exporter<T>::export_cons(interp, (package_name + "::" + name).c_str(), constructor, state);
		}
//		void export_raw(const char* name, void (*raw_func)(interpreter* me_perl, CV* cef), void* extra = 0, unsigned extra_length = sizeof(void*)) {
//			implementation::export_as(interp, (package_name + "::" + name).c_str(), raw_func, extra, extra_length);
//		}
	};

	namespace implementation {
		namespace scalar {
			template<class T1, class T2, class T3, class T4, class T5> const Ref<Any>::Temp Base::tie(const char* package_name, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5) {
				Package package(package_name);
				Ref<Any>::Temp tier(package.call("TIESCALAR", t1, t2, t3, t4, t5), override());
				tie_to(tier);
				return tier;
			}
		}
		namespace array {
			template<class T1, class T2, class T3, class T4, class T5> const Ref<Any>::Temp Value::tie(const char* package_name, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5) {
				Package package(package_name);
				Ref<Any>::Temp tier(package.call("TIEARRAY", t1, t2, t3, t4, t5), override());
				tie_to(tier);
				return tier;
			}
		}
		namespace hash {
			template<class T1, class T2, class T3, class T4, class T5> const Ref<Any>::Temp Value::tie(const char* package_name, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5) {
				Package package(package_name);
				Ref<Any>::Temp tier(package.call("TIEHASH", t1, t2, t3, t4, t5), override());
				tie_to(tier);
				return tier;
			}
		}

		template<typename T, typename A1, typename A2, typename A3, typename A4, typename A5> class constructor;
		template<typename T> class constructor<T, null_type, null_type, null_type, null_type, null_type> {
			static T* contruct() {
				return new T();
			}
		};
		template<typename T, typename A1> struct constructor<T, A1, null_type, null_type, null_type, null_type> {
			static T* construct(const A1& arg1) {
				return new T(arg1);
			}
		};
		template<typename T, typename A1, typename A2> struct constructor<T, A1, A2, null_type, null_type, null_type> {
			static T* construct(const A1& arg1, const A2& arg2) {
				return new T(arg1, arg2);
			}
		};
		template<typename T, typename A1, typename A2, typename A3> struct constructor<T, A1, A2, A3, null_type, null_type> {
			static T* construct(const A1& arg1, const A2& arg2, const A3& arg3) {
				return new T(arg1, arg2, arg3);
			}
		};
		template<typename T, typename A1, typename A2, typename A3, typename A4> struct constructor<T, A1, A2, A3, A4, null_type> {
			static T* construct(const A1& arg1, const A2& arg2, const A3& arg3, const A4& arg4) {
				return new T(arg1, arg2, arg3, arg4);
			}
		};
		template<typename T, typename A1, typename A2, typename A3, typename A4, typename A5> struct constructor {
			static T* construct(const A1& arg1, const A2& arg2, const A3& arg3, const A4& arg4, const A5& arg5) {
				return new T(arg1, arg2, arg3, arg4, arg5);
			}
		};

		template<typename T> struct destructor {
			static int destroy(interpreter* interp, SV* var, MAGIC* magic) {
				Object_buffer& tmp = *get_magic_ptr<Object_buffer>(magic);
				if (tmp.owns) {
					delete tmp.get<T>();
				}
				return 0;
			}
		};

		namespace classes {
			class Temp {
				public:
				Package package;
				Temp(const Interpreter& interp, const char* classname);
			};

			class Type_info {
				const std::type_info& raw;
				Type_info& operator=(Type_info&);
				public:
				Type_info(const std::type_info&);
//				Type_info(const Type_info&);
				bool operator<(const Type_info&) const;
				const char* name() const;
			};
			class Table {
				std::map<Type_info, MGVTBL*> table;
				void add(const Type_info&, int (*)(interpreter*, SV*, MAGIC*));
				public:
				~Table();
				template<typename T> MGVTBL* get(){
					Type_info key(typeid(T));
					if (table.find(key) == table.end()) {
						add(key, destructor<T>::destroy);
					}
					return table[key];
				};
			};
			static Table magic_table;
		}
	}

	template<typename A1 = implementation::null_type, typename A2 = implementation::null_type, typename A3 = implementation::null_type, typename A4 = implementation::null_type, typename A5 = implementation::null_type> struct init {
	};

	template<typename T> class Class {
		Package package;
		bool hash;
		typedef implementation::classes::State State;
		static MGVTBL* get_magic_table() {
			return implementation::classes::magic_table.get<T>();
		}
		State& get_class_data() {
			return *reinterpret_cast<State*>(implementation::get_magic_ptr(package.interp, reinterpret_cast<SV*>(package.stash), sizeof(State)));
		}
		public:
		Class(const implementation::classes::Temp& other) : package(other.package) {
			SV* stash = reinterpret_cast<SV*>(package.stash);
			if (! implementation::has_magic_string(package.interp, stash)) {
				implementation::classes::State info(package.get_name().c_str(), get_magic_table());
				implementation::set_magic_object(package.interp, stash, info);
			}
		}
		explicit Class(const Package& _package) : package(_package) {
		}
		template<typename U> typename boost::enable_if<typename boost::is_function<typename boost::remove_pointer<U>::type >::type, void>::type add(const char * name, const U& function) {
				package.export_sub(name, function);
		}
		template<typename U> typename boost::enable_if<typename boost::is_member_function_pointer<U>::type, void>::type add(const char * name, const U& method) {
				package.export_method(name, method);
		}
		template<typename A1, typename A2, typename A3, typename A4, typename A5> void add(const char* name, const init<A1, A2, A3, A4, A5>&) {
			typedef typename implementation::constructor<T, A1, A2, A3, A4, A5> constructor;
			State& state = get_class_data();
			package.export_constructor<T>("new", constructor::construct, state);
//			package.export_raw("DESTROY", &implementation::destructor<T>::destroy);
		}
		template<typename A1, typename A2, typename A3, typename A4, typename A5> void add(const init<A1, A2, A3, A4, A5>& foo) {
			add("new", foo);
		}
	};


	/*
	 * TODO:
	 * get rid of consts?
	 * package variables
	 * fix Array::Value so list isn't necessary?
	 */
	class Interpreter {
		const boost::shared_ptr<interpreter> raw_interp;
		public:
		Hash::Temp modglobal;

		private:
		Interpreter& operator=(const Interpreter&); //What should that do?
		Interpreter(interpreter*);
		public:
		Interpreter();

		Interpreter clone() const;
		friend bool operator==(const Interpreter& first, const Interpreter& second);
		interpreter* get_interpreter() const;
		void report() const;
		void set_context() const;

		const Scalar::Temp eval(const char*);
		const Scalar::Temp eval(const Scalar::Base&);
		const Array::Temp eval_list(const char*);
		const Array::Temp eval_list(const Scalar::Base&);
		Package use(const char* package_name);
		Package use(const char* package_name, double version);

		const Package get_package(const char* name) const;
		Package get_package(const char* name);

		const Scalar::Temp scalar(const char*) const;
		Scalar::Temp scalar(const char*);
		const Array::Temp array(const char*) const;
		Array::Temp array(const char*);
		const Hash::Temp hash(const char* name) const;
		Hash::Temp hash(const char* name);

		const Glob glob(const char*) const;
		Glob glob(const char*);

		Scalar::Temp undef() const;
		Integer::Temp value_of(int) const;
		Uinteger::Temp value_of(unsigned) const;
		Number::Temp value_of(double) const;
		String::Temp value_of(Raw_string) const;
		String::Temp value_of(const char*) const;
		String::Temp value_of(const std::string&) const;
		template<typename T> Ref<Any>::Temp value_of(const T* object) const {
			return implementation::get_from_cache(raw_interp, object);
		}

		Ref<Glob>::Temp open(Raw_string);

		template <typename T> const Ref<Code>::Temp export_sub(const char* name, T& fptr) {
			return take_ref(implementation::export_sub(raw_interp.get(), name, fptr));
		}
		template <typename T> const Code::Value export_flat(const char* name, T& fptr) {
			return implementation::export_flatsub(raw_interp.get(), name, fptr);
		}
		implementation::classes::Temp add_class(const char* name) {
			return implementation::classes::Temp(*this, name);
		}

		const Array::Temp list();
		template<typename T1> const Array::Temp list(const T1& t1) {
			Array::Temp ret = list();
			ret.push(t1);
			return ret;
		}
		template<typename T1, typename T2> const Array::Temp list(const T1& t1, const T2& t2) {
			Array::Temp ret = list();
			ret.push(t1, t2);
			return ret;
		}
		template<typename T1, typename T2, typename T3> const Array::Temp list(const T1& t1, const T2& t2, const T3 t3) {
			Array::Temp ret = list();
			ret.push(t1, t2, t3);
			return ret;
		}
		template<typename T1, typename T2, typename T3, typename T4> const Array::Temp list(const T1& t1, const T2& t2, const T3 t3, const T4 t4) {
			Array::Temp ret = list();
			ret.push(t1, t2, t3, t4);
			return ret;
		}
		const Hash::Temp hash();
		
		template<typename T1> Scalar::Temp call(const char* name, const T1& t1) {
			return implementation::Call_stack(get_interpreter()).push(t1).sub_scalar(name);
		}
		template<typename T1, typename T2> Scalar::Temp call(const char* name, const T1& t1, const T2& t2) {
			return implementation::Call_stack(get_interpreter()).push(t1, t2).sub_scalar(name);
		}
		template<typename T1, typename T2, typename T3> Scalar::Temp call(const char* name, const T1& t1, const T2& t2, const T3& t3) {
			return implementation::Call_stack(get_interpreter()).push(t1, t2, t3).sub_scalar(name);
		}
		template<typename T1, typename T2, typename T3, typename T4> Scalar::Temp call(const char* name, const T1& t1, const T2& t2, const T3& t3, const T4& t4) {
			return implementation::Call_stack(get_interpreter()).push(t1, t2, t3, t4).sub_scalar(name);
		}

		template<typename T1> String::Temp pack(const Raw_string pattern, const T1& t1) {
			return implementation::Call_stack(get_interpreter()).push(t1).pack(pattern);
		}
		template<typename T1, typename T2> String::Temp pack(const Raw_string pattern, const T1& t1, const T2 t2) {
			return implementation::Call_stack(get_interpreter()).push(t1, t2).pack(pattern);
		}
		template<typename T1, typename T2, typename T3> String::Temp pack(const Raw_string pattern, const T1& t1, const T2 t2, const T3& t3) {
			return implementation::Call_stack(get_interpreter()).push(t1, t2, t3).pack(pattern);
		}
		template<typename T1, typename T2, typename T3, typename T4> String::Temp pack(const Raw_string pattern, const T1& t1, const T2 t2, const T3& t3, const T4& t4) {
			return implementation::Call_stack(get_interpreter()).push(t1, t2, t3, t4).pack(pattern);
		}
	};
}
