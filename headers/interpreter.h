namespace perl {
	namespace typecast {
		template<typename T, typename E> struct typemap {
			typedef boost::false_type from_type;
		};
	}

	namespace implementation {
		template<typename T> struct typemap_from_info {
			typedef typeof(typecast::typemap<T>::cast_from) from_type;
			typedef typename boost::function_traits<from_type>::result_type result_type;
			typedef typename boost::function_traits<from_type>::arg2_type arg_type;
		};

		template<typename T> struct typemap_to_info {
			typedef typeof(typecast::typemap<T>::cast_to) to_type;
			typedef typename boost::function_traits<to_type>::result_type result_type;
			typedef typename boost::function_traits<to_type>::arg1_type arg_type;
		};
	}

	template<typename T> const typename implementation::typemap_to_info<T>::result_type typecast_to(const typename implementation::typemap_to_info<T>::arg_type& t) {
		return typecast::typemap<T>::cast_to(t);
	}

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
	namespace implementation {
		extern const char to_eval[];
		/*
		 * Magic subroutine functions
		 */
		const Raw_string get_magic_string(interpreter*, SV*);
		void set_magic_string(interpreter*, SV*, Raw_string);
		void set_magic_string(interpreter*, SV*, const void*, unsigned length);

		void* get_magic_ptr(interpreter*, SV*, int);
		void* get_magic_object_impl(interpreter*, SV*, int);
		template<typename T> T* get_magic_object(const Scalar::Base& var) {
			return *static_cast<T**>(get_magic_object_impl(var.interp, var.get_SV(false), sizeof(T*)));
		}
		template<typename T> T& get_magic_buffer(interpreter* interp, SV* var) {
			return *static_cast<T*>(get_magic_object_impl(interp, var, sizeof(T)));
		}
		template<typename T> T& get_magic_buffer(const Scalar::Base& var) {
			return get_magic_buffer<T>(var.interp, var.get_SV(false));
		}

		bool has_magic_string(interpreter*, SV*);

		typedef int (*magic_fun)(interpreter*, SV*, MAGIC*);
		void attach_getset_magic(interpreter* interp, SV* var, magic_fun get_val, magic_fun set_val, const void* buffer, size_t buffer_length);

		void* get_magic_ptr(const MAGIC*);
		template<typename T> T* get_magic_ptr(const MAGIC* magic) {
			return static_cast<T*>(get_magic_ptr(magic));
		}

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
				tmp = (T)val;
				return 0;
			}
		}
		struct Object_buffer {
			void* ref;
			const std::set<const std::type_info*>& types;
			bool owns;
			Object_buffer(void* _ref, const std::set<const std::type_info*>& _types, bool _owns) : ref(_ref), types(_types), owns(_owns) {
			}
			template<typename T> T* get() {
				return static_cast<T*>(ref);
			}
		};

		struct Class_state {
			const char* classname;
			MGVTBL* const magic_table;
			const std::type_info& type;
			bool is_persistent;
			bool use_hash;
			Class_state(const char*, const std::type_info&, MGVTBL*, bool, bool);
			std::set<const std::type_info*> family;
			private:
			Class_state(const Class_state&);
			Class_state& operator=(const Class_state&);
		};

	}

	namespace magical {
		using namespace implementation::magic;
		template<typename T, typename U> static void readonly(const Scalar::Base& var, T& object, void (T::*get_value)(U&)) {
			const Wrapper<T, U> funcs(object, get_value, read_type());
			implementation::attach_getset_magic(var.interp, var.get_SV(false), Wrapper<T, U>::read, NULL, &funcs, sizeof(funcs));
		}
		template<typename T, typename U> static void writeonly(const Scalar::Base& var, T& object, void (T::*set_value)(U&)) {
			const Wrapper<T, U> funcs(object, set_value, write_type());
			implementation::attach_getset_magic(var.interp, var.get_SV(false), NULL, Wrapper<T, U>::write, &funcs, sizeof(funcs));
		}
		template<typename T, typename U, typename V> static void readwrite(const Scalar::Base& var, T& object, void (T::*get_value)(U&), void (T::*set_value)(V&)) {
			const Wrapper<T, U, V> funcs(object, get_value, set_value);
			implementation::attach_getset_magic(var.interp, var.get_SV(false), Wrapper<T, U, V>::read, Wrapper<T, U, V>::write, &funcs, sizeof(funcs));
		}

		template<typename T> static void readonly(const Scalar::Base& var, T& object) {
			implementation::attach_getset_magic(var.interp, var.get_SV(false), var_read<T>, NULL, &object, 0);
		}
		template<typename T> static void writeonly(const Scalar::Base& var, T& object) {
			implementation::attach_getset_magic(var.interp, var.get_SV(false), NULL, var_write<T>, &object, 0);
		}
		template<typename T> static void readwrite(const Scalar::Base& var, T& object) {
			implementation::attach_getset_magic(var.interp, var.get_SV(false), var_read<T>, var_write<T>, &object, 0);
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
		unsigned get_num_args() const;
		const Scalar::Temp call(const char* name);
		const Scalar::Temp call(const Ref<Code>::Value& name);
		context get_context() const;
	};

	namespace implementation {
		const Code::Value export_as(interpreter*, const char* name, void (*)(interpreter*, CV*), const void*, int);
		template<typename T> const Code::Value export_as(interpreter* interp, const char* name, void(*glue)(interpreter*, CV*), const T& func) {
			return export_as(interp, name, glue, &func, sizeof func);
		}

		template<typename T> const T& get_function_pointer(interpreter* interp, CV* cef) {
			Raw_string ret = get_magic_string(interp, reinterpret_cast<SV*>(cef));
			if (ret.length < sizeof(T)) {
				throw Runtime_exception("Magical error!");
			}
			return *reinterpret_cast<const T*>(ret.value);
		}

		SV* value_of_pointer(interpreter*, const void*, const std::type_info&);
		template<typename T> const Scalar::Temp value_of_pointer(interpreter* interp, T* pointer) {
			return Scalar::Temp(interp, value_of_pointer(interp, pointer, typeid(T)), false);
		}
		Ref<Any>::Temp store_in_cache(interpreter*, const void*, const implementation::Class_state&);

		void die(interpreter*, const char* message);

#define TRY_OR_THROW(a) try {\
			a;\
		}\
		catch(std::exception& e) {\
			/* TODO: make it throw an object */ \
			die(me_perl, e.what());\
		}\
		catch(...) {\
			std::cerr << "Cought unknown exception, terminating" << std::endl;\
			std::terminate();\
		}

		//Section functions

		template<typename R, typename A> struct export_stack {
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
		template<typename R, typename A1, typename = void> struct export_sub_1;
		template<typename R, typename A1> struct export_sub_1<R, A1, typename boost::disable_if<typename boost::is_convertible<Array::Temp, A1>::type>::type> {
			typedef R (*func_ptr)(A1);
			static void subroutine(interpreter* me_perl, CV* cef) {
				Argument_stack arg_stack(me_perl);
				const func_ptr ref = implementation::get_function_pointer<func_ptr>(me_perl, cef);
				TRY_OR_THROW(arg_stack.returns(ref(typecast_to<A1>(arg_stack[0]))));
			}
		};
		template<typename R, typename A1> struct export_sub_1<R, A1, typename boost::enable_if<typename boost::is_convertible<Array::Temp, A1>::type>::type> {
			typedef R (*func_ptr)(A1);
			static void subroutine(interpreter* me_perl, CV* cef) {
				Argument_stack arg_stack(me_perl);
				const func_ptr ref = implementation::get_function_pointer<func_ptr>(me_perl, cef);
				Array::Temp arg = arg_stack.get_arg();
				TRY_OR_THROW(arg_stack.returns(ref(arg)));
			}
		};

		template<typename R, typename A1, typename A2> struct export_sub_2 {
			typedef R (*func_ptr)(A1, A2);
			static void subroutine(interpreter* me_perl, CV* cef) {
				Argument_stack arg_stack(me_perl);
				const func_ptr ref = implementation::get_function_pointer<func_ptr>(me_perl, cef);
				TRY_OR_THROW(arg_stack.returns(ref(typecast_to<A1>(arg_stack[0]), typecast_to<A2>(arg_stack[1]))));
			}
		};
		template<typename R, typename A1, typename A2, typename A3> struct export_sub_3 {
			typedef R (*func_ptr)(A1, A2);
			static void subroutine(interpreter* me_perl, CV* cef) {
				Argument_stack arg_stack(me_perl);
				const func_ptr ref = implementation::get_function_pointer<func_ptr>(me_perl, cef);
				TRY_OR_THROW(arg_stack(ref(typecast_to<A1>(arg_stack[0]), typecast_to<A2>(arg_stack[1]), typecast_to<A3>(arg_stack[2]))));
			}
		};

		template<typename T> struct export_vstack {
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
		template<typename, typename= void> struct export_sub_v1;
		template<typename A1> struct export_sub_v1<A1, typename boost::disable_if<typename boost::is_convertible<Array::Temp, A1>::type>::type> {
			typedef void(*func_ptr)(A1);
			static void subroutine(interpreter* me_perl, CV* cef) {
				Argument_stack arg_stack(me_perl);
				const func_ptr ref = implementation::get_function_pointer<func_ptr>(me_perl, cef);
				ref(typecast_to<A1>(arg_stack[0]));
			}
		};
		template<typename A1> struct export_sub_v1<A1, typename boost::enable_if<typename boost::is_convertible<Array::Temp, A1>::type>::type> {
			typedef void(*func_ptr)(A1);
			static void subroutine(interpreter* me_perl, CV* cef) {
				Argument_stack arg_stack(me_perl);
				const func_ptr ref = implementation::get_function_pointer<func_ptr>(me_perl, cef);
				Array::Temp arg = arg_stack.get_arg();
				ref(static_cast<A1>(arg));
			}
		};

		template<typename A1, typename A2> struct export_sub_v2 {
			typedef void(*func_ptr)(A1, A2);
			static void subroutine(interpreter* me_perl, CV* cef) {
				Argument_stack arg_stack(me_perl);
				const func_ptr ref = implementation::get_function_pointer<func_ptr>(me_perl, cef);
				ref(typecast_to<A1>(arg_stack[0]), typecast_to<A2>(arg_stack[1]));
			}
		};

		template<typename A1, typename A2, typename A3> struct export_sub_v3 {
			typedef void(*func_ptr)(A1, A2, A3);
			static void subroutine(interpreter* me_perl, CV* cef) {
				Argument_stack arg_stack(me_perl);
				const func_ptr ref = implementation::get_function_pointer<func_ptr>(me_perl, cef);
				ref(typecast_to<A1>(arg_stack[0], typecast_to<A2>(me_perl, arg_stack[1]), typecast_to<A3>(me_perl, arg_stack[3])));
			}
		};

		template<typename R, typename A1> const Code::Value export_stacksub(interpreter* interp, const char* name, R (fptr)(A1)) {
			return export_as(interp, name, export_stack<R, A1>::subroutine, fptr);
		}
		template<typename A> const Code::Value export_stacksub(interpreter* interp, const char* name, void (fptr)(A)) {
			return export_as(interp, name, export_vstack<A>::subroutine, fptr);
		}

		template<typename R> const Code::Value export_sub(interpreter* interp, const char* name, R (fptr)()) {
			return export_as(interp, name, export_sub_0<R>::subroutine, fptr);
		}
		static inline const Code::Value export_sub(interpreter* interp, const char* name, void (fptr)()) {
			return export_as(interp, name, export_sub_v0::subroutine, fptr);
		}

		template<typename R, typename A1> const Code::Value export_sub(interpreter* interp, const char* name, R (fptr)(A1)) {
			return export_as(interp, name, export_sub_1<R, A1>::subroutine, fptr);
		}
		template<typename A1> const Code::Value export_sub(interpreter* interp, const char* name, void (fptr)(A1)) {
			return export_as(interp, name, export_sub_v1<A1>::subroutine, fptr);
		}

		template<typename R, typename A1, typename A2> const Code::Value export_sub(interpreter* interp, const char* name, R (fptr)(A1, A2)) {
			return export_as(interp, name, export_sub_2<R, A1, A2>::subroutine, fptr);
		}
		template<typename A1, typename A2> const Code::Value export_sub(interpreter* interp, const char* name, void (fptr)(A1, A2)) {
			return export_as(interp, name, export_sub_v2<A1, A2>::subroutine, fptr);
		}

		template<typename R, typename A1, typename A2, typename A3> const Code::Value export_sub(interpreter* interp, const char* name, R (fptr)(A1, A2, A3)) {
			return export_as(interp, name, export_sub_3<R, A1, A2, A3>::subroutine, fptr);
		}

		template<typename A1, typename A2, typename A3> const Code::Value export_sub(interpreter* interp, const char* name, void (fptr)(A1, A2, A3)) {
			return export_as(interp, name, export_sub_v3<A1, A2, A3>::subroutine, fptr);
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

		template<typename R, typename T, typename A1, typename = void> struct export_method_1;
		template<typename R, typename T, typename A1> struct export_method_1<R, T, A1, typename boost::disable_if<typename boost::is_convertible<Array::Temp, A1>::type>::type> {
			typedef R (T::*func_ptr)(A1);
			static void method(interpreter* me_perl, CV* cef) {
				Argument_stack arg_stack(me_perl);
				const func_ptr ref = implementation::get_function_pointer<func_ptr>(me_perl, cef);
				TRY_OR_THROW(arg_stack.returns((get_magic_object<T>(arg_stack[0])->*ref)(typecast_to<A1>(arg_stack[1]))));
			}
		};

		template<typename R, typename T, typename A1> struct export_method_1<R, T, A1, typename boost::enable_if<typename boost::is_convertible<Array::Temp, A1>::type>::type> {
			typedef R (T::*func_ptr)(A1);
			static void method(interpreter* me_perl, CV* cef) {
				Argument_stack arg_stack(me_perl);
				const func_ptr ref = implementation::get_function_pointer<func_ptr>(me_perl, cef);
				Array::Temp arg = arg_stack.get_arg();
				arg.shift();
				TRY_OR_THROW(arg_stack.returns((get_magic_object<T>(arg_stack[0])->*ref)(arg)));
			}
		};

		template<typename T, typename A1, typename = void> struct export_method_v1;
		template<typename T, typename A1> struct export_method_v1<T, A1, typename boost::disable_if<typename boost::is_convertible<Array::Temp, A1>::type>::type> {
			typedef void (T::*func_ptr)(A1);
			static void method(interpreter* me_perl, CV* cef) {
				Argument_stack arg_stack(me_perl);
				const func_ptr ref = implementation::get_function_pointer<func_ptr>(me_perl, cef);
				TRY_OR_THROW((get_magic_object<T>(arg_stack[0])->*ref)(typecast_to<A1>(arg_stack[1])));
			}
		};
		template<typename T, typename A1> struct export_method_v1<T, A1, typename boost::enable_if<typename boost::is_convertible<Array::Temp, A1>::type>::type> {
			typedef void (T::*func_ptr)(A1);
			static void method(interpreter* me_perl, CV* cef) {
				Argument_stack arg_stack(me_perl);
				const func_ptr ref = implementation::get_function_pointer<func_ptr>(me_perl, cef);
				Array::Temp arg = arg_stack.get_arg();
				arg.shift();
				TRY_OR_THROW((get_magic_object<T>(arg_stack[0])->*ref)(arg));
			}
		};

		template<typename T, typename R> static void export_method(interpreter* const interp, const char* name, R (T::* const fptr)() const) {
			implementation::export_as(interp, name, export_method_0<R,T>::method, fptr);
		}
		template<typename T> static void export_method(interpreter* const interp, const char* name, void (T::* const fptr)() const) {
			implementation::export_as(interp, name, export_method_v0<T>::method, fptr);
		}
		template<typename T, typename R, typename A1> static void export_method(interpreter* const interp, const char* name, R (T::* const fptr)(A1) const) {
			implementation::export_as(interp, name, export_method_1<R, T, A1>::method, fptr);
		}
		template<typename T, typename A1> static void export_method(interpreter* const interp, const char* name, void (T::* const fptr)(A1) const) {
			implementation::export_as(interp, name, export_method_v1<T, A1>::method, fptr);
		}

		template<typename T, typename R> static void export_method(interpreter* const interp, const char* name, R (T::* const fptr)()) {
			implementation::export_as(interp, name, export_method_0<R,T>::method, fptr);
		}
		template<typename T> static void export_method(interpreter* const interp, const char* name, void (T::* const fptr)()) {
			implementation::export_as(interp, name, export_method_v0<T>::method, fptr);
		}
		template<typename T, typename R, typename A1> static void export_method(interpreter* const interp, const char* name, R (T::* const fptr)(A1)) {
			implementation::export_as(interp, name, export_method_1<R, T, A1>::method, fptr);
		}
		template<typename T, typename A1> static void export_method(interpreter* const interp, const char* name, void (T::* const fptr)(A1)) {
			implementation::export_as(interp, name, export_method_v1<T, A1>::method, fptr);
		}

		//Section member variables

		template<typename T, typename A> struct export_member_ptr {
			typedef A T::* const memb_ptr;
			static void method(interpreter* me_perl, CV* cef) {
				Argument_stack arg_stack(me_perl);
				const memb_ptr ref = implementation::get_function_pointer<memb_ptr>(me_perl, cef);
				if (arg_stack.get_num_args() == 0) {
					die(me_perl, "Fatal error");//FIXME description
				}
				else if (arg_stack.get_num_args() == 1) {
					TRY_OR_THROW(arg_stack.returns(get_magic_object<T>(arg_stack[0])->*ref));
				}
				else {
					TRY_OR_THROW(arg_stack.returns(get_magic_object<T>(arg_stack[0])->*ref = arg_stack[1]));
				}
			}
		};
		template<typename T, typename A> static void export_member(interpreter* const interp, const char* name, A T::* const member) {
			implementation::export_as(interp, name, export_member_ptr<T, A>::method, member);
		}

		//Section constructors

		struct constructor_info {
			void* fptr;
			const Class_state& class_state;
			constructor_info(void* _fptr, const Class_state& _state) : fptr(_fptr), class_state(_state) {
			}
			template<typename T> constructor_info(T* _fptr, const Class_state& _state) : fptr(reinterpret_cast<void*>(_fptr)), class_state(_state) {
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
					TRY_OR_THROW(arg_stack.returns(store_in_cache(me_perl, ref(typecast_to<A1>(arg_stack[1])), data.class_state)));
				}
			};
			template<typename A1, typename A2> struct arg2 {
				typedef T (*func_ptr)(A1, A2);
				static void subroutine(interpreter* me_perl, CV* cef) {
					Argument_stack arg_stack(me_perl);
					const constructor_info data = implementation::get_function_pointer<constructor_info>(me_perl, cef);
					const func_ptr ref = data.get<func_ptr>();
					TRY_OR_THROW(arg_stack.returns(store_in_cache(me_perl, ref(typecast_to<A1>(arg_stack[1]), typecast_to<A2>(arg_stack[2])), data.class_state)));
				}
			};
			template<typename A1, typename A2, typename A3> struct arg3 {
				typedef T (*func_ptr)(A1, A2);
				static void subroutine(interpreter* me_perl, CV* cef) {
					Argument_stack arg_stack(me_perl);
					const constructor_info data = implementation::get_function_pointer<constructor_info>(me_perl, cef);
					const func_ptr ref = data.get<func_ptr>();
					TRY_OR_THROW(arg_stack.returns(store_in_cache(me_perl, ref(typecast_to<A1>(arg_stack[1]), typecast_to<A2>(arg_stack[2]), typecast_to<A3>(arg_stack[3])), data.class_state)));
				}
			};
			public:
			static const Code::Value export_cons(interpreter* interp, const char* name, T* (fptr)(), const Class_state& state) {
				return export_as(interp, name, arg0::subroutine, constructor_info(fptr, state));
			}
			template<typename A1> static const Code::Value export_cons(interpreter* interp, const char* name, T* (*fptr)(A1), const Class_state& state) {
				return export_as(interp, name, arg1<A1>::subroutine, constructor_info(fptr, state));
			}
			template<typename A1, typename A2> static const Code::Value export_cons(interpreter* interp, const char* name, T* (fptr)(A1, A2), const Class_state& state) {
				return export_as(interp, name, arg2<A1, A2>::subroutine, constructor_info(fptr, state));
			}
			template<typename A1, typename A2, typename A3> static const Code::Value export_cons(interpreter* interp, const char* name, T* (fptr)(A1, A2, A3), const Class_state& state) {
				return export_as(interp, name, arg3<A1, A2, A3>::subroutine, constructor_info(fptr, state));
			}
		};
#undef TRY_OR_THROW

		template<typename T, typename A1, typename A2, typename A3, typename A4, typename A5> class constructor;
		template<typename T> struct constructor<T, null_type, null_type, null_type, null_type, null_type> {
			static T* construct() {
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

		template<typename T> int destructor(interpreter* , SV* , MAGIC* magic) {
			Object_buffer& tmp = *get_magic_ptr<Object_buffer>(magic);
			if (tmp.owns) {
				delete tmp.get<T>();
			}
			return 0;
		}
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
		Package(interpreter*, const char*, bool = false);
		Package(interpreter*, SV*, bool = false);
		const std::string& get_name() const;
		operator const std::string&() const;
		Scalar::Temp scalar(const char*) const;
		Array::Temp array(const char*) const;
		Hash::Temp hash(const char*) const;
		
		template<typename U> typename boost::enable_if<typename boost::is_function<typename boost::remove_pointer<U>::type >::type, const Ref<Code>::Temp>::type add(const char * name, const U& function) {
			return implementation::export_sub(interp, (package_name + "::" + name).c_str(), function).take_ref();
		}
		template<typename T> Code::Value add_stacksub(const char* name, const T& func) {
			return implementation::export_stacksub(interp, (package_name + "::" + name).c_str(), func);
		}

		private:
		template<typename T, typename U> typename boost::enable_if<typename boost::is_member_function_pointer<U T::*>::type, void>::type add(const char* name, U T::* const method) {
			implementation::export_method(interp, (package_name + "::" + name).c_str(), method);
		}
		template<typename T, typename U> typename boost::enable_if<typename boost::is_member_object_pointer<U T::*>::type, void>::type add(const char * name, U T::* const member) {
			implementation::export_member(interp, (package_name + "::" + name).c_str(), member);
		}
		template<typename T, typename U> void export_constructor(const char* name, const U& constructor, const implementation::Class_state& state) {
			implementation::constructor_exporter<T>::export_cons(interp, (package_name + "::" + name).c_str(), constructor, state);
		}
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

		class Class_temp {
			public:
			Package package;
			bool persistence;
			bool use_hash;
			Class_temp(interpreter* interp, const char* classname);
			Class_temp& is_persistent(bool = true);
			Class_temp& uses_hash(bool = true);
		};
		MGVTBL* get_object_vtbl(const std::type_info& type, int (*destruct_ptr)(interpreter*, SV*, MAGIC*));

		Class_state& register_type(interpreter*, const char*, const std::type_info&, MGVTBL*, bool, bool);
	}

	template<typename A1 = implementation::null_type, typename A2 = implementation::null_type, typename A3 = implementation::null_type, typename A4 = implementation::null_type, typename A5 = implementation::null_type> struct init {
	};

	template<typename T> class Class {
		Package package;
		bool hash;
		typedef implementation::Class_state State;
		State& get_class_data() {
			return *static_cast<State*>(implementation::get_magic_ptr(package.interp, reinterpret_cast<SV*>(package.stash), sizeof(State)));
		}
		public:
		void initialize(bool _is_persistent, bool _use_hash) {
			SV* stash = reinterpret_cast<SV*>(package.stash);
			if (! implementation::has_magic_string(package.interp, stash)) {
				const State& info = implementation::register_type(package.interp, package.get_name().c_str(), typeid(T), implementation::get_object_vtbl(typeid(T), implementation::destructor<T>), _is_persistent, _use_hash);
				implementation::set_magic_string(package.interp, stash, &info, 0);
			}
		}
		Class(const implementation::Class_temp& other) : package(other.package) {
			initialize(other.persistence, other.use_hash);
		}
		explicit Class(const Package& _package) : package(_package) {
			initialize(false, false);
		}
		template<typename U> typename boost::enable_if<typename boost::is_function<typename boost::remove_pointer<U>::type >::type, void>::type add(const char * name, const U& function) {
			package.add(name, function);
		}
		template<typename U> void add(const char * name, U T::* const member) {
			package.add(name, member);
		}
		template<typename A1, typename A2, typename A3, typename A4, typename A5> void add(const char* name, const init<A1, A2, A3, A4, A5>&) {
			typedef typename implementation::constructor<T, A1, A2, A3, A4, A5> constructor;
			State& state = get_class_data();
			package.export_constructor<T>(name, constructor::construct, state);
		}
		template<typename A1, typename A2, typename A3, typename A4, typename A5> void add(const init<A1, A2, A3, A4, A5>& foo) {
			add("new", foo);
		}

		bool& uses_hash() {
			return get_class_data().use_hash;
		}
		bool& is_persistent() {
			return get_class_data().is_persistent;
		}
		void add_parent(const Class<T>& parent) {
			const std::set<const std::type_info*>& parents = parent.get_class_data().family;
			get_class_data().family.insert(parents.begin(), parents.end());
			package.array("ISA").push(parent.get_name());
		}
		void add_parent(const char* parent_name); //TODO
	};


	/*
	 * TODO:
	 * get rid of consts?
	 * package variables
	 * fix Array::Value so list isn't necessary?
	 */
	class Interpreter {
		const boost::shared_ptr<interpreter> raw_interp;
		Interpreter& operator=(const Interpreter&);
		public:
		Interpreter(interpreter*, const override&);
		Interpreter();
		Interpreter(int, const char*[]);

		Interpreter clone() const;
		friend bool operator==(const Interpreter& first, const Interpreter& second);
		interpreter* get_interpreter() const;
		void report() const;
		void set_context() const;
		Hash::Temp modglobal() const;
		int run() const;


		const Scalar::Temp eval(const char*) const;
		const Scalar::Temp eval(const Scalar::Base&) const;
		const Array::Temp eval_list(const char*) const;
		const Array::Temp eval_list(const Scalar::Base&) const;
		Package use(const char* package_name) const;
		Package use(const char* package_name, double version) const;

		Package package(const char* name) const;

		Scalar::Temp scalar(const char*) const;
		Array::Temp array(const char*) const;
		Hash::Temp hash(const char* name) const;

		Glob glob(const char*) const;

		const Regex regex(const String::Value&) const;
		const Regex regex(const char*) const;

		const Scalar::Temp undef() const;
		const Integer::Temp value_of(int) const;
		const Uinteger::Temp value_of(unsigned) const;
		const Number::Temp value_of(double) const;
		const String::Temp value_of(Raw_string) const;
		const String::Temp value_of(const char*) const;
		const String::Temp value_of(const std::string&) const;
		template<typename T, typename U> const typename implementation::typemap_from_info<T>::result_type value_of(const T& t, const U* = static_cast<Any*>(0)) const {
			return typecast::typemap<T>::cast_from(raw_interp.get(), t);
		}

		Handle open(Raw_string) const;
		Handle in() const;
		Handle out() const;
		Handle err() const;

		template<typename U> typename boost::enable_if<typename boost::is_function<typename boost::remove_pointer<U>::type >::type, const Ref<Code>::Temp>::type add(const char * name, const U& function) {
			return implementation::export_sub(raw_interp.get(), name, function).take_ref();
		}
		template<typename T> const Code::Value add_stacksub(const char* name, const T& fptr) const {
			return implementation::export_stacksub(raw_interp.get(), name, fptr);
		}
		const implementation::Class_temp add_class(const char* name) const {
			return implementation::Class_temp(raw_interp.get(), name);
		}
		template<typename T> typename boost::disable_if<typename boost::is_pointer<T>::type, Scalar::Temp>::type add(const char* name, T& variable) {
			Scalar::Temp ret = scalar(name);
			magical::readwrite(ret, variable);
			return ret;
		}

		const Array::Temp list() const;
		template<typename T1> const Array::Temp list(const T1& t1) const {
			Array::Temp ret = list();
			ret.push(t1);
			return ret;
		}
		template<typename T1, typename T2> const Array::Temp list(const T1& t1, const T2& t2) const {
			Array::Temp ret = list();
			ret.push(t1, t2);
			return ret;
		}
		template<typename T1, typename T2, typename T3> const Array::Temp list(const T1& t1, const T2& t2, const T3 t3) const {
			Array::Temp ret = list();
			ret.push(t1, t2, t3);
			return ret;
		}
		template<typename T1, typename T2, typename T3, typename T4> const Array::Temp list(const T1& t1, const T2& t2, const T3 t3, const T4 t4) const {
			Array::Temp ret = list();
			ret.push(t1, t2, t3, t4);
			return ret;
		}
		const Hash::Temp hash() const;
		
		const Scalar::Temp call(const char* name) const {
			return implementation::Call_stack(get_interpreter()).sub_scalar(name);
		}
		template<typename T1> const Scalar::Temp call(const char* name, const T1& t1) const {
			return implementation::Call_stack(get_interpreter()).push(t1).sub_scalar(name);
		}
		template<typename T1, typename T2> const Scalar::Temp call(const char* name, const T1& t1, const T2& t2) const {
			return implementation::Call_stack(get_interpreter()).push(t1, t2).sub_scalar(name);
		}
		template<typename T1, typename T2, typename T3> const Scalar::Temp call(const char* name, const T1& t1, const T2& t2, const T3& t3) const {
			return implementation::Call_stack(get_interpreter()).push(t1, t2, t3).sub_scalar(name);
		}
		template<typename T1, typename T2, typename T3, typename T4> const Scalar::Temp call(const char* name, const T1& t1, const T2& t2, const T3& t3, const T4& t4) const {
			return implementation::Call_stack(get_interpreter()).push(t1, t2, t3, t4).sub_scalar(name);
		}

		template<typename T1> String::Temp pack(const Raw_string pattern, const T1& t1) const {
			return implementation::Call_stack(get_interpreter()).push(t1).pack(pattern);
		}
		template<typename T1, typename T2> String::Temp pack(const Raw_string pattern, const T1& t1, const T2 t2) const {
			return implementation::Call_stack(get_interpreter()).push(t1, t2).pack(pattern);
		}
		template<typename T1, typename T2, typename T3> String::Temp pack(const Raw_string pattern, const T1& t1, const T2 t2, const T3& t3) const {
			return implementation::Call_stack(get_interpreter()).push(t1, t2, t3).pack(pattern);
		}
		template<typename T1, typename T2, typename T3, typename T4> String::Temp pack(const Raw_string pattern, const T1& t1, const T2 t2, const T3& t3, const T4& t4) const {
			return implementation::Call_stack(get_interpreter()).push(t1, t2, t3, t4).pack(pattern);
		}
	};

	template<typename T> const typename implementation::typemap_from_info<T>::result_type typecast_from(Interpreter& interp, const T& t) {
		return typecast::typemap<T>::cast_from(interp, t);
	}
	template<typename T> const typename implementation::typemap_from_info<T>::result_type typecast_from(interpreter* pre_interp, const T& t) {
		Interpreter interp(pre_interp, override());
		return typecast_from<T>(interp, t);
	}

	namespace typecast {
		template<typename T> struct exported_type {
			typedef boost::true_type from_type;
			static const Scalar::Temp cast_from(Interpreter& interp, const T& value) {
				return implementation::value_of_pointer(interp.get_interpreter(), &value);
			}
			static const T& cast_to(const Scalar::Value& value) {
				return *implementation::get_magic_object<T>(value);
			}
		};
	}

}
