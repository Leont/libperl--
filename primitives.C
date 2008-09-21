#include "internal.h"
#include "perl++.h"

namespace perl {
	namespace implementation {
		/*
		 * Class Undefined::Value
		 */
		
		Undefined::Undefined(interpreter* _interp, SV* _value) : Scalar::Base(interp, _value) {
		}
		bool Undefined::is_compatible_type(const Scalar::Base& val) {
			return SvTYPE(val.get_SV(false)) == SVt_NULL;
		}
		const std::string& Undefined::cast_error() {
			static const std::string message("Not undefined");
			return message;
		}

		/*
		 * Class Integer::Value
		 */

		Integer::Integer(interpreter* _interp, SV* _handle) : Scalar::Base(_interp, _handle) {
		}
		Integer& Integer::operator=(const Integer& other) {
			sv_setpviv_mg(get_SV(false), other.int_value());
			return *this;
		}
		Integer& Integer::operator=(int other) {
			sv_setpviv_mg(get_SV(false), other);
			return *this;
		}
		Integer& Integer::operator+=(int other) {
			return operator=(int_value() + other);
		}
		Integer& Integer::operator-=(int other) {
			return operator=(int_value() - other);
		}
		Integer& Integer::operator*=(int other) {
			return operator=(int_value() * other);
		}
		Integer& Integer::operator/=(int other) {
			return operator=(int_value() / other);
		}
		Integer& Integer::operator%=(int other) {
			return operator=(int_value() % other);
		}

		Integer& Integer::operator++() {
			sv_inc(get_SV(true));
			return *this;
		}
		Integer Integer::operator++(int) {
			const int ret = int_value();
			++*this;
			return Integer(interp, newSViv(ret));
		}
		Integer& Integer::operator--() {
			sv_dec(get_SV(true));
			return *this;
		}
		Integer Integer::operator--(int) {
			const int ret = int_value();
			--*this;
			return Integer(interp, newSViv(ret));
		}

		Integer::operator int() const {
			return int_value();
		}
		int Integer::int_value() const {
			return SvIV(get_SV(true));
		}
		bool Integer::operator==(const Integer& right) const {
			return int_value() == right.int_value();
		}
		bool operator!=(const Integer& left, const Integer& right) {
			return !(left == right);
		}

		bool Integer::is_compatible_type(const Scalar::Base& val) {
			return SvIOK(val.get_SV(false));
		}
		const std::string& Integer::cast_error() {
			static const std::string message("Not an Integer");
			return message;
		}

		/*
		 * Class Uinteger::Value
		 */

		Uinteger::Uinteger(interpreter* _interp, SV* _handle) : Scalar::Base(_interp, _handle) {
		}
		Uinteger& Uinteger::operator=(const Uinteger& other) {
			sv_setuv_mg(get_SV(false), other.unsigned_value());
			return *this;
		}
		Uinteger& Uinteger::operator=(unsigned other) {
			sv_setuv_mg(get_SV(false), other);
			return *this;
		}
		Uinteger& Uinteger::operator+=(unsigned other) {
			return operator=(unsigned_value() + other);
		}
		Uinteger& Uinteger::operator-=(unsigned other) {
			return operator=(unsigned_value() - other);
		}
		Uinteger& Uinteger::operator*=(unsigned other) {
			return operator=(unsigned_value() * other);
		}
		Uinteger& Uinteger::operator/=(unsigned other) {
			return operator=(unsigned_value() / other);
		}
		Uinteger& Uinteger::operator%=(unsigned other) {
			return operator=(unsigned_value() % other);
		}

		Uinteger& Uinteger::operator++() {
			sv_inc(get_SV(true));
			return *this;
		}
		Uinteger Uinteger::operator++(int) {
			const unsigned ret = unsigned_value();
			++*this;
			return Uinteger(interp, newSVuv(ret));
		}
		Uinteger& Uinteger::operator--() {
			sv_dec(get_SV(true));
			return *this;
		}
		Uinteger Uinteger::operator--(int) {
			const unsigned ret = unsigned_value();
			--*this;
			return Uinteger(interp, newSVuv(ret));
		}

		Uinteger::operator unsigned() const {
			return unsigned_value();
		}
		unsigned Uinteger::unsigned_value() const {
			return SvUV(get_SV(true));
		}
		bool Uinteger::operator==(const Uinteger& right) const {
			return unsigned_value() == right.unsigned_value();
		}

		bool operator!=(const Uinteger& left, const Uinteger& right) {
			return !(left == right);
		}
		bool Uinteger::is_compatible_type(const Scalar::Base& val) {
			return SvUOK(val.get_SV(false));
		}
		const std::string& Uinteger::cast_error() {
			static const std::string message("Not an unsigned Integer");
			return message;
		}

		/*
		 * Class Number::value
		 */

		Number::Number(interpreter* _interp, SV* _handle) : Scalar::Base(_interp, _handle) {
		}
		Number& Number::operator=(const Number& other) {
			sv_setnv_mg(get_SV(false), other.number_value());
			return *this;
		}
		Number& Number::operator=(double other) {
			sv_setnv_mg(get_SV(false), other);
			return *this;
		}
		Number& Number::operator+=(double other) {
			return operator=(number_value() + other);
		}
		Number& Number::operator-=(double other) {
			return operator=(number_value() - other);
		}
		Number& Number::operator*=(double other) {
			return operator=(number_value() * other);
		}
		Number& Number::operator/=(double other) {
			return operator=(number_value() / other);
		}

		Number::operator double() const {
			return number_value();
		}
		double Number::number_value() const {
			return SvNV(get_SV(true));
		}

		bool Number::is_compatible_type(const Scalar::Base& val) {
			return SvNOK(val.get_SV(false)) || Integer::is_compatible_type(val) || Uinteger::is_compatible_type(val);
		}
		const std::string& Number::cast_error() {
			static const std::string message("Not a Number");
			return message;
		}
		
		/*
		 * Class String::Value
		 */

		String::String(interpreter* _interp, SV* _handle) : Scalar::Base(_interp, _handle) {
		}
		String& String::operator=(const String& other) {
			SvSetMagicSV(get_SV(false), other.get_SV(true));
			return *this;
		}
		String& String::operator=(const std::string& other) {
			sv_setpvn_mg(get_SV(false), other.c_str(), other.length());
			return *this;
		}
		String& String::operator=(Raw_string new_value) {
			sv_setpvn_mg(get_SV(false), new_value.value, new_value.length);
			return *this;
		}
		String& String::operator=(const char* new_value) {
			sv_setpv_mg(get_SV(false), new_value);
			return *this;
		}

		String::operator const Raw_string() const {
			return string_value();
		}
		const char* String::get_raw() const {
			STRLEN len;
			return SvPVx(get_SV(true), len);
		}
//		String::operator const char*() const {
//			return get_raw();
//		}
		String::operator bool() const {
			return length() > 0;
		}
		unsigned String::length() const {
			return sv_len(get_SV(true)); // Unicode!?
		}
		String& String::operator+=(const String& other) {
			sv_catsv_mg(get_SV(false), other.get_SV(true));
			return *this;
		}
		String& String::operator+=(const std::string& other) {
			sv_catpvn_mg(get_SV(false), other.c_str(), other.length());
			return *this;
		}
		String& String::operator+=(Raw_string other) {
			sv_catpvn_mg(get_SV(false), other.value, other.length);
			return *this;
		}
		String& String::operator+=(const char* other) {
			sv_catpv_mg(get_SV(false), other);
			return *this;
		}
		
		bool String::operator==(const String& right) const {
			return sv_eq(get_SV(true), right.get_SV(true));
		}
		bool String::operator==(const std::string& right) const {
			return strnEQ(get_raw(), right.c_str(), std::min(length(), right.length()) + 1);
		}
		bool String::operator==(const char* right) const {
			return strnEQ(get_raw(), right, length() + 1);
		}

		
		bool operator==(const std::string& left, const String& right) {
			return right == left;
		}
		bool operator==(const char* left, const String& right) {
			return right == left;
		}

		bool operator!=(const String& left, const String& right) {
			return !(left == right);
		}
		bool operator!=(const String& left, const std::string& right) {
			return !(left == right);
		}
		bool operator!=(const std::string& left, const String& right) {
			return !(left == right);
		}

		bool operator!=(const String& left, const char* right) {
			return !(left == right);
		}
		bool operator!=(const char* left, const String& right) {
			return !(left == right);
		}

		void String::replace(unsigned offset, unsigned sublength, const char* other, unsigned other_length) {
			sv_insert(get_SV(true), offset, sublength, const_cast<char*>(other), other_length);
		}
		void String::replace(unsigned offset, unsigned sublength, Raw_string other) {
			replace(offset, sublength, other.value, other.length);
		}
		void String::insert(unsigned offset, Raw_string other) {
			replace(offset, -1, other.value, other.length);
		}
		void String::insert(unsigned offset, const char* other, unsigned other_length) {
			replace(offset, -1, other, other_length);
		}

		const Array::Temp String::unpack(const Raw_string pattern) const {
			return implementation::Call_stack(interp).unpack(pattern, *this);
		}
		Package String::get_package(bool create) const {
			return Package(interp, get_SV(true), create);
		}

		bool String::match(const perl::Regex& regex) const {
			return regex.match(*this);
		}
		bool String::substitute(const perl::Regex& regex, const String& replacement) {
			return regex.substitute(*this, replacement);
		}
		bool String::substitute(const perl::Regex& regex, Raw_string replacement) {
			return regex.substitute(*this, replacement);
		}

		SV* String::copy(const Scalar::Base& other) {
			interpreter* const interp = other.interp;
			STRLEN len;
			const char* tmp = SvPVx(other.get_SV(true), len);
			SV* ret = newSVpvn(tmp, len);
			if (SvUTF8(other.get_SV(false))) {
				SvUTF8_on(ret);
			}
			return ret;
		}
		bool String::is_compatible_type(const Scalar::Base& val) {
			return SvPOK(val.get_SV(false)) || Number::is_compatible_type(val);
		}
		const std::string& String::cast_error() {
			static const std::string message("Not a String");
			return message;
		}
	}
}
