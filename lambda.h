#include <boost/lambda/lambda.hpp>
#include <boost/lambda/casts.hpp>

namespace boost {
	namespace lambda {
		template<class Act, class Y> struct plain_return_type_2<arithmetic_action<Act>, Y, perl::Scalar::Temp> {
			typedef typename perl::implementation::nearest_arithmetic_type<Y>::type type;
		};
		template<class Act, class Y> struct plain_return_type_2<arithmetic_action<Act>, perl::Scalar::Temp, Y> {
			typedef typename perl::implementation::nearest_arithmetic_type<Y>::type type;
		};
		template<class Act, class Y> struct plain_return_type_2<pre_increment_decrement_action<Act>, perl::Scalar::Temp, Y> {
			typedef perl::Scalar::Value& type;
		};
//		template<class Act> struct plain_return_type_2<arithmetic_action<Act>, perl::Scalar::Temp, std::string> {
//			typedef std::string type;
//		};
		template<class Act, class Y> struct plain_return_type_2<arithmetic_assignment_action<Act>, perl::Scalar::Temp, Y> {
			typedef perl::Scalar::Value& type;
		};
	}
}

namespace perl {
	using boost::lambda::_1;
	using boost::lambda::_2;
	using boost::lambda::_3;

	using boost::lambda::ll_static_cast;
	using boost::lambda::ll_dynamic_cast;
	using boost::lambda::ll_const_cast;
	using boost::lambda::ll_reinterpret_cast;
}

