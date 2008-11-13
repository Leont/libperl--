namespace perl {
	class lock;
	class Glob;

	/*
	 * Section Array
	 */
	class Array;
	namespace implementation {
		namespace array {
			class Value;
		}
		namespace hash {
			class Value;
		}
	}

	namespace implementation {
		namespace array {
			typedef int key_type;

			/*
			 * Class Array::Const_iterator
			 */
			class Const_iterator {
				const Value& ref;
				key_type index;
				public:
				Const_iterator(const Value&, key_type);

				typedef std::random_access_iterator_tag iterator_category;
				typedef const Scalar::Temp value_type;
				typedef ptrdiff_t difference_type;
				typedef Const_iterator* pointer;
				typedef Const_iterator& reference;

				Const_iterator& operator++();
				Const_iterator& operator--();
				Const_iterator operator++(int);
				Const_iterator operator--(int);
				
				Const_iterator& operator+=(difference_type);
				Const_iterator& operator-=(difference_type);
				Const_iterator operator+(difference_type) const;
				Const_iterator operator-(difference_type) const;
				difference_type operator-(const Const_iterator&) const;
				value_type operator[](difference_type) const;

				const Scalar::Temp operator*() const;
				friend bool operator==(const Const_iterator&, const Const_iterator&);
				friend bool operator!=(const Const_iterator&, const Const_iterator&);
			};

			/*
			 * Class Array::Iterator
			 */
			class Iterator {
				Value& ref;
				key_type index;
				public:
				Iterator(Value&, key_type);

				typedef std::random_access_iterator_tag iterator_category;
				typedef Scalar::Temp value_type;
				typedef ptrdiff_t difference_type;
				typedef Iterator* pointer;
				typedef Iterator& reference;

				Iterator& operator++();
				Iterator& operator--();
				Iterator operator++(int);
				Iterator operator--(int);

				Iterator& operator+=(difference_type);
				Iterator& operator-=(difference_type);
				Iterator operator+(difference_type) const;
				Iterator operator-(difference_type) const;
				difference_type operator-(const Iterator&) const;
				Scalar::Temp operator[](difference_type) const;

				Scalar::Temp operator*() const;
				friend bool operator==(const Iterator&, const Iterator&);
				friend bool operator!=(const Iterator&, const Iterator&);
				operator Const_iterator() const;
			};
			typedef std::reverse_iterator<Iterator>       Reverse_iterator;
			typedef std::reverse_iterator<Const_iterator> Const_reverse_iterator;

			/*
			 * Class Array::Length
			 * Represents the length of an array
			 */

			class Length {
				Value& array;
				friend class Value;
				public:
				Length(const Length&);
				explicit Length(Value& _array);
				operator unsigned() const;
				Length& operator=(unsigned new_length);
			};

			/*
			 * Class Array::Value
			 * Implements the behavior expected from an array
			 */
			class Value {
				Value(const Value&);
				protected:
				interpreter* const interp;
				AV* const handle;
				Value(interpreter*, AV*);
				public:

				Value& operator=(const Value&);

				interpreter* get_interpreter() const;
				void push(const Scalar::Base&);
				void push(const Scalar::Temp&);
				void push(const Value&);
				void push(IV);
				void push(UV);
				void push(NV);
				template<typename T> typename boost::enable_if<typename boost::is_arithmetic<T>::type>::type push(T value) {
					push(static_cast<typename nearest_arithmetic_type<T>::type>(value));
				}
				void push(const char*);
				void push(Raw_string);
				template<typename T1, typename T2> void push(const T1& t1,const T2& t2) {
					push(t1);
					push(t2);
				}
				template<typename T1, typename T2,typename T3> void push(const T1& t1,const T2& t2, const T3& t3) {
					push(t1, t2);
					push(t3);
				}
				template<typename T1, typename T2,typename T3, typename T4> void push(const T1& t1,const T2& t2, const T3& t3, const T4& t4) {
					push(t1, t2);
					push(t3, t4);
				}
				template<typename T1, typename T2,typename T3, typename T4, typename T5> void push(const T1& t1,const T2& t2, const T3& t3, const T4& t4, const T5& t5) {
					push(t1, t2, t3);
					push(t4, t5);
				}
				template<typename T1, typename T2,typename T3, typename T4, typename T5, typename T6> void push(const T1& t1,const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6) {
					push(t1, t2, t3);
					push(t4, t5, t6);
				}
				template<typename T1, typename T2,typename T3, typename T4, typename T5, typename T6, typename T7> void push(const T1& t1,const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7) {
					push(t1, t2, t3, t4);
					push(t5, t6, t7);
				}
				template<typename T1, typename T2,typename T3, typename T4, typename T5, typename T6, typename T7, typename T8> void push(const T1& t1,const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7, const T8& t8) {
					push(t1, t2, t3, t4);
					push(t5, t6, t7, t8);
				}
				const Scalar::Temp pop();
				const Scalar::Temp shift();
				private:
				void unshift_prepare(unsigned);
				public:
				void unshift(const Scalar::Base&);
				void unshift(IV);
				void unshift(UV);
				void unshift(NV);
				template<typename T> typename boost::enable_if<typename boost::is_arithmetic<T>::type>::type unshift(T value) {
					unshift(static_cast<typename nearest_arithmetic_type<T>::type>(value));
				}
				void unshift(const char*);
				void unshift(Raw_string);
				void unshift(const Value&);
				void unshift(const Array&);
				//TODO: Template unshifts?
				
				const Scalar::Temp operator[](int) const;
				Scalar::Temp operator[](int);

				const Temp reverse() const; //TODO

				const Scalar::Temp remove(int);
				bool exists(int) const;

				unsigned length() const;
				Length length();
				void clear();
				void undefine();
				void extend(unsigned);

				const perl::String::Temp pack(const Raw_string) const;
				
				void tie_to(const Scalar::Base&);
				void untie(); //TODO
				template<typename T1, typename T2, typename T3, typename T4, typename T5> const Ref<Any>::Temp tie(const char* package_name, const T1& t1 = null_type(), const T2& t2 = null_type(), const T3& t3 = null_type(), const T4& t4 = null_type(), const T5& t5 = null_type());
				const Scalar::Temp tied() const;

				const scalar::Temp_template<reference::Nonscalar<Array> > take_ref() const;

				template<typename T> void each(const T& functor) const {
					const key_type size = length();
					for(key_type current = 0; current < size; ++current) {
						functor(operator[](current));
					}
				}
				template<typename T> void each(const T& functor) {
					const key_type size = length();
					for(key_type current = 0; current < size; ++current) {
						Scalar::Temp tmp = operator[](current);
						functor(tmp);
					}
				}

				template<typename T> void each_index(const T& functor) const {
					const key_type size = length();
					for(key_type current = 0; current < size; ++current) {
						functor(current, operator[](current));
					}
				}
				template<typename T> void each_index(const T& functor) {
					const key_type size = length();
					for(key_type current = 0; current < size; ++current) {
						functor(current, operator[](current));
					}
				}

				template<typename T> Temp map(const T& functor) const;
				template<typename T> Temp grep(const T& functor) const;

				template<typename T, typename U> T reduce(const U& functor, T ret = T()) const {
					const key_type size = length();
					for (key_type current = 0; current < size; ++current) {
						ret = functor(ret, operator[](current));
					}
					return ret;
				}

				//Begin TODO
				template<typename T> const Scalar::Temp first(const T& functor) const {
					const key_type size = length();
					for (key_type current = 0; current < size; ++current) {
						if (functor(operator[](current))) {
							return operator[](current);
						}
//						return interp.undef();//...
					}
				}
				const Scalar::Temp max() const;
				const Scalar::Temp min() const;
				const Temp shuffled() const;
				const int sum() const;
				//Endof TODO
				template<typename T> bool any(const T& functor) const {
					const key_type size = length();
					for (key_type current = 0; current < size; ++current) {
						if (functor(operator[](current))) {
							return true;
						}
					}
					return false;
				}
				template<typename T> bool all(const T& functor) const {
					const key_type size = length();
					for (key_type current = 0; current < size; ++current) {
						if ( !functor(operator[](current))) {
							return false;
						}
					}
					return size != 0;
				}
				template<typename T> bool none(const T& functor) const {
					const key_type size = length();
					for (key_type current = 0; current < size; ++current) {
						if ( functor(operator[](current))) {
							return false;
						}
					}
					return size != 0;
				}
				template<typename T> bool notall(const T& functor) const {
					const key_type size = length();
					for (key_type current = 0; current < size; ++current) {
						if ( !functor(operator[](current))) {
							return true;
						}
					}
					return false;
				}
				unsigned true_count() const;
				unsigned false_count() const;

				Const_iterator begin() const;
				Const_iterator end() const;

				Iterator begin();
				Iterator end();

				Const_reverse_iterator rbegin() const;
				Const_reverse_iterator rend() const;

				Reverse_iterator rbegin();
				Reverse_iterator rend();

				static const std::string& cast_error();

				friend class perl::Array;
				friend class Length;
				friend class perl::Glob;
				friend class perl::lock;
			};

			class Temp : public Value {
				friend class perl::Array;
				friend class Value;
				friend class hash::Value;
				mutable bool owns;
				explicit Temp(interpreter*);
				public:
				Temp(const Temp&);
				using Value::operator=;
				Temp(interpreter*, AV*, bool);
				void release() const;
				~Temp();
			};
		}

        template<typename T> struct perl_type<T, typename boost::enable_if<typename boost::is_base_of<array::Value, T>::type>::type> {
            typedef boost::true_type type;
        };

	}

	/*
	 * Class Array
	 * An Array variable.
	 */
	class Array : public implementation::array::Value {
		public:
		typedef implementation::array::key_type key_type;

		typedef implementation::array::Value Value;
		typedef implementation::array::Temp Temp;
		typedef implementation::array::Length Length;

		Array(const Array&);
		Array(const Temp&);
		//TODO template constructors?

		public:
		~Array();
		static bool is_storage_type(const Any::Temp&);

		typedef implementation::array::Const_iterator         Const_iterator;
		typedef implementation::array::Iterator               Iterator;
		typedef implementation::array::Reverse_iterator       Reverse_iterator;
		typedef implementation::array::Const_reverse_iterator Const_reverse_iterator;
		using Value::operator=;
	};

	template<typename T> inline Array::Temp implementation::array::Value::map(const T& functor) const {
		const Array::key_type size = length();
		Array::Temp ret(interp);
		for (key_type current = 0; current < size; ++current) {
			ret.push(functor(operator[](current)));
		}
		return ret;
	}

	template<typename T> inline Array::Temp implementation::array::Value::grep(const T& functor) const {
		const Array::key_type size = length();
		Array::Temp ret(interp);
		for(key_type current = 0; current < size; ++current) {
			const Scalar::Temp tmp = operator[](current);
			if (functor(static_cast<const Scalar::Value&>(tmp))) {
				ret.push(tmp);
			}
		}
		return ret;
	}
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

				static bool is_compatible_type(const Scalar::Base& var);
			};
		}
	}
}
