namespace perl {
	namespace typecast {
		template<typename T> struct is_pushable {
			typedef typename boost::mpl::or_<typename boost::is_convertible<Scalar::Temp, T>::type, typename typemap<T>::from_type>::type type;
		};

		template<> struct typemap<Scalar::Value> {
			typedef boost::false_type from_type;
		};
		template<typename T> struct typemap<T, typename boost::enable_if<typename boost::is_convertible<const Scalar::Value, T>::type>::type> {
			typedef boost::false_type from_type;
			typedef boost::true_type to_type;

			static const T& cast_to(const T& t) {
				return t;
			}
			static const Scalar::Temp cast_from(Interpreter& interp, const T& t) {
				return interp.value_of(t);
			}
		};

		template<typename T, typename U> struct typemap< std::pair<T, U>, void> {
			typedef typename boost::mpl::and_<typename is_pushable<T>::type, typename is_pushable<U>::type>::type from_type;

			static const Ref<Array>::Temp cast_from(Interpreter& interp, const std::pair<T, U>& t) {
				return interp.list(typecast_from(interp, t.first), typecast_from(interp, t.second)).take_ref();
			}

			static const std::pair<T, U> cast_to(const Scalar::Value& scalar) {
				return std::pair<T, U>(typecast_to<T>(scalar[0]), typecast_to<U>(scalar[1]));
			}
		};

		template<typename T> struct typemap< std::vector<T> > {
			typedef typename boost::enable_if<typename is_pushable<T>::type>::type from_type;

			static const Array::Temp cast_from(Interpreter& interp, const std::vector<T>& array) {
				Array::Temp ret = interp.list();
				for (int i = 0; i < array.length(); ++i) {
					ret.push(array[i]);
				}
				return ret;
			}

			static const std::vector<T> cast_to(const Array::Value& array) {
				std::vector<T> ret;
				for (int i = 0; i < array.length(); ++i) {
					ret.push_back(array[i]);
				}
				return ret;
			}
			static const std::vector<T> cast_to(const Scalar::Value& scalar) {
				Ref<Array> array = scalar;
				return cast_to(*array);
			}
		};
	}
}
