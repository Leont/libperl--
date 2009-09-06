namespace perl {
	namespace typecast {
		template<typename T, typename E> struct typemap {
			typedef boost::false_type from_type;
		};
		template<typename T> struct typemap<T, typename boost::enable_if<typename boost::is_convertible<const Scalar::Value, T>::type>::type> {
			typedef boost::false_type from_type;
			typedef boost::true_type to_type;

			static const T& cast_to(const T& t) {
				return t;
			}
			static const Scalar::Temp cast_from(interpreter*, const T& t);
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

	template<typename T> const T typecast_to(const typename implementation::typemap_to_info<T>::arg_type& t) {
		return typecast::typemap<T>::cast_to(t);
	}
	template<typename T> const typename implementation::typemap_from_info<T>::result_type typecast_from(interpreter* interp, const T& t) {
		return typecast::typemap<T>::cast_from(interp, t);
	}

	namespace typecast {
		template<typename T> struct is_pushable {
			typedef typename boost::is_convertible<Scalar::Temp, T>::type type;
		};

		template<typename T, typename U> struct typemap< std::pair<T, U>, void> {
//			typedef typename typename boost::mpl::and_<typename is_pushable<T>::type, typename is_pushable<U>::type>::type from_type;
			typedef boost::true_type from_type;

			static const Ref<Array>::Temp cast_from(interpreter* pre_interp, const std::pair<T, U>& t);

			static const std::pair<T, U> cast_to(const Scalar::Value& scalar) {
				return std::pair<T, U>(typecast_to<T>(scalar[0]), typecast_to<U>(scalar[1]));
			}
		};

		template<typename T> struct typemap< std::vector<T> > {
			typedef typename boost::enable_if<typename is_pushable<T>::type>::type from_type;

//			static const Array
		};
	}
	namespace implementation {
		/*
		template<typename T> struct is_pair {
			typedef boost::false_type type;
		};

		template<typename T, typename U> struct is_pair< std::pair<T, U> > {
			typedef boost::true_type type;
			typedef T left_type;
			typedef U right_type;
		};

		template<typename T> typename boost::enable_if< typename is_pair<T>::type, T>::type typecast_to(const Scalar::Value& scalar) {
			return T(typecast_to<typename is_pair<T>::left_type>(scalar[0]), typecast_to<typename is_pair<T>::right_type>(scalar[1]));
		}
		*/
	}
}
