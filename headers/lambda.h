#include <boost/lambda/lambda.hpp>
#include <boost/lambda/casts.hpp>
#include <boost/lambda/bind.hpp>

namespace boost {
	namespace lambda {
		template<class Act, class Y> struct plain_return_type_2<arithmetic_action<Act>, Y, typename boost::enable_if<typename boost::is_arithmetic<Y>::type, perl::Scalar::Temp>::type> {
			typedef typename perl::implementation::nearest_arithmetic_type<Y>::type type;
		};
		template<class Act, class Y> struct plain_return_type_2<arithmetic_action<Act>, typename boost::enable_if<typename boost::is_arithmetic<Y>::type, perl::Scalar::Temp>::type, Y> {
			typedef typename perl::implementation::nearest_arithmetic_type<Y>::type type;
		};
		template<class Act, class Y> struct plain_return_type_2<pre_increment_decrement_action<Act>, perl::Scalar::Temp, Y> {
			typedef perl::Scalar::Value& type;
		};
		template<class Act, class Y> struct plain_return_type_2<arithmetic_assignment_action<Act>, perl::Scalar::Temp, Y> {
			typedef perl::Scalar::Value& type;
		};

		struct is_action;
		struct ok_action;
		template<typename T> struct test_action;

		template<> struct test_action<is_action> {
			template<typename RET, typename A1, typename A2, typename A3> static RET apply(const A1& left, const A2& right, const A3& message) {
				return is(left, right, message);
			}
		};
		template<> struct test_action<ok_action> {
			template<typename RET, typename A1, typename A2> static RET apply(const A1& assertion, const A2& message) {
				return ok(assertion, message);
			}
		};
		template<typename T, typename A> struct return_type_N<test_action<T>, A> {
			typedef bool type;
		};

		namespace detail {
			template<typename T, typename U, typename V> struct is_tuple {
				typedef boost::tuple<typename reference_argument<const T>::type, typename reference_argument<const U>::type, typename const_copy_argument<V>::type> type;
			};
			template<typename T, typename U> struct ok_tuple {
				typedef boost::tuple<typename reference_argument<const T>::type, typename reference_argument<const U>::type> type;
			};
			template<typename T, typename U, typename V> struct is_base {
				typedef lambda_functor_base<
					action<3, test_action<is_action> >,
					typename is_tuple<T, U, V>::type
				> type;
			};
			template<typename T, typename U> struct ok_base {
				typedef lambda_functor_base<
					action<2, test_action<ok_action> >,
					typename ok_tuple<T, U>::type
				> type;
			};
		}

		template<typename T, typename U, typename V> const lambda_functor< typename detail::is_base<T, U, V>::type >
		ll_is(const T& left, const U& right, const V& message) {
			return typename detail::is_base<T, U, V>::type(typename detail::is_tuple<T, U, V>::type(left, right, message));
		}
		template<typename T, typename U, typename V> const const_parameter_lambda_functor<lambda_functor< typename detail::is_base<T, U, V>::type > >
		ll_is_c(const T& left, const U& right, const V& message) {
			return const_parameters(ll_is(left, right, message));
		}
		template<typename T, typename U> const lambda_functor< typename detail::ok_base<T, U>::type >
		ll_ok(const T& assertion, const U& message) {
			return typename detail::ok_base<T, U>::type(typename detail::ok_tuple<T, U>::type(assertion, message));
		}
		template<typename T, typename U> const const_parameter_lambda_functor<lambda_functor< typename detail::ok_base<T, U>::type > >
		ll_ok_c(const T& assertion, const U& message) {
			return const_parameters(ll_ok(assertion, message));
		}
	}
}

namespace perl {
	using boost::lambda::_1;
	using boost::lambda::_2;
	using boost::lambda::_3;
	using boost::lambda::bind;
	using boost::lambda::make_const;


	using boost::lambda::ll_static_cast;
	using boost::lambda::ll_dynamic_cast;
	using boost::lambda::ll_const_cast;
	using boost::lambda::ll_reinterpret_cast;

	using boost::lambda::ll_is;
	using boost::lambda::ll_ok;
}

