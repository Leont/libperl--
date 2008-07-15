namespace perl {
	class lock;

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
	const implementation::scalar::Temp_template<implementation::reference::Nonscalar<Array> > take_ref(const implementation::array::Value&);

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
				Length(const Length&);
				public:
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
				void push(int);
				void push(unsigned);
				void push(double);
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
				void unshift(int);
				void unshift(unsigned);
				void unshift(double);
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
				
				template<typename T> void each(const T& functor) const {
					const key_type size = length();
					for(key_type current = 0; current < size; ++current) {
						functor(operator[](current));
					}
				}
				template<typename T> void each(const T& functor) {
					const key_type size = length();
					for(key_type current = 0; current < size; ++current) {
						functor(operator[](current));
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

				template<typename T, typename U> U reduce(const T& functor, U ret = U()) const {
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
				friend const implementation::scalar::Temp_template<implementation::reference::Nonscalar<Array> > perl::take_ref(const Value&);

				friend class perl::lock;
			};

			class Temp : public Value {
				friend class perl::Array;
				friend class Value;
				friend class hash::Value;
				mutable bool owns;
				Temp& operator=(const Temp&);
				explicit Temp(interpreter*);
				public:
				Temp(const Temp&);
				Temp(interpreter*, AV*, bool);
				void release() const;
				~Temp();
			};
		}
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

		explicit Array(interpreter*);
		//TODO template constructors?
		Array(const Array&);
		Array(const Temp&);

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

	/*
	 * End of Section Array
	 */


	/*
	 * Section Hash
	 */

	class Hash;
	const implementation::scalar::Temp_template<implementation::reference::Nonscalar<Hash> > take_ref(const implementation::hash::Value&);
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

				bool exists(Raw_string);
				bool exists(const Scalar::Base&);
				const Scalar::Temp erase(Raw_string);
				const Scalar::Temp erase(const Scalar::Base&);
				void clear();
				void undefine();

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
				const Array::Temp keys() const; //TODO
				const Array::Temp values() const; //TODO

				static const std::string& cast_error();

				friend class perl::Hash;
				friend const scalar::Temp_template< reference::Nonscalar<Hash> > perl::take_ref(const Value&);
				friend class perl::lock;
			};

			class Iterator::Key_type {
				const Iterator& ref;
				Key_type& operator=(const Key_type&);
				public:
				explicit Key_type(const Iterator& _ref);
				Raw_string as_raw_string() const;
				const Scalar::Temp as_scalar() const;
				operator Raw_string() const;
				operator const char*() const;
				operator const Scalar::Temp() const;
			};
			std::ostream& operator<<(std::ostream&, const Iterator::Key_type&);
		}
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

		explicit Hash(const Interpreter&);
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

	//TODO: everything
	class Glob {
		interpreter* const interp;
		const GV* const value;
		public:
		Glob(const Glob&);
//		Glob(const rvalue&);
		Glob& operator=(const Glob&);

		Glob& operator=(const Scalar::Base&);
		Glob& operator=(const Array::Value&);
		Glob& operator=(const Hash::Value&);
		Glob& operator=(const Code::Value&);
		~Glob();

		Raw_string name() const;
		const Scalar::Temp scalar_value() const;
		Array::Temp array_value() const;
		Hash::Temp hash_value() const;
		const Code::Value code_value() const;
	};
	
}