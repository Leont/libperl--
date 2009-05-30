#include "internal.h"
#include "perl++.h"

namespace perl {
	/*
	 * Class Undefined::Value
	 */
	
	Undefined::Value::Undefined(interpreter* _interp, SV* _value) : Scalar::Base(_interp, _value) {
	}
	bool Undefined::Value::is_compatible_type(const Scalar::Base& val) {
		return SvTYPE(val.get_SV(false)) == SVt_NULL;
	}
	const std::string& Undefined::Value::cast_error() {
		static const std::string message("Not undefined");
		return message;
	}

	/*
	 * Class Integer::Value
	 */

	Integer::Value::Integer(interpreter* _interp, SV* _handle) : Scalar::Base(_interp, _handle) {
	}
	Integer::Value& Integer::Value::operator=(const perl::Integer::Value& other) {
		sv_setpviv_mg(get_SV(false), other.int_value());
		return *this;
	}
	Integer::Value& Integer::Value::operator=(IV other) {
		sv_setpviv_mg(get_SV(false), other);
		return *this;
	}
	Integer::Value& Integer::Value::operator+=(IV other) {
		return operator=(int_value() + other);
	}
	Integer::Value& Integer::Value::operator-=(IV other) {
		return operator=(int_value() - other);
	}
	Integer::Value& Integer::Value::operator*=(IV other) {
		return operator=(int_value() * other);
	}
	Integer::Value& Integer::Value::operator/=(IV other) {
		return operator=(int_value() / other);
	}
	Integer::Value& Integer::Value::operator%=(IV other) {
		return operator=(int_value() % other);
	}

	Integer::Value& Integer::Value::operator++() {
		sv_inc(get_SV(true));
		return *this;
	}
	Integer::Temp Integer::Value::operator++(int) {
		const int ret = int_value();
		++*this;
		return perl::Integer::Temp(interp, newSViv(ret), true);
	}
	Integer::Value& Integer::Value::operator--() {
		sv_dec(get_SV(true));
		return *this;
	}
	Integer::Temp Integer::Value::operator--(int) {
		const int ret = int_value();
		--*this;
		return perl::Integer::Temp(interp, newSViv(ret), true);
	}

	Integer::Value::operator IV() const {
		return int_value();
	}
	IV Integer::Value::int_value() const {
		return SvIV(get_SV(true));
	}
	bool Integer::Value::operator==(const perl::Integer::Value& right) const {
		return int_value() == right.int_value();
	}
	bool operator!=(const Integer::Value& left, const Integer::Value& right) {
		return !(left == right);
	}

	SV* Integer::Value::copy(const Scalar::Base& orig) {
		interpreter* interp = orig.interp;
		return newSViv(orig.int_value());
	}
	bool Integer::Value::is_compatible_type(const Scalar::Base& val) {
		return SvIOK(val.get_SV(false));
	}
	const std::string& Integer::Value::cast_error() {
		static const std::string message("Not an Integer::Value");
		return message;
	}

	/*
	 * Class Uinteger::Value
	 */

	Uinteger::Value::Uinteger(interpreter* _interp, SV* _handle) : Scalar::Base(_interp, _handle) {
	}
	Uinteger::Value& Uinteger::Value::operator=(const perl::Uinteger::Value& other) {
		sv_setuv_mg(get_SV(false), other.unsigned_value());
		return *this;
	}
	Uinteger::Value& Uinteger::Value::operator=(UV other) {
		sv_setuv_mg(get_SV(false), other);
		return *this;
	}
	Uinteger::Value& Uinteger::Value::operator+=(UV other) {
		return operator=(unsigned_value() + other);
	}
	Uinteger::Value& Uinteger::Value::operator-=(UV other) {
		return operator=(unsigned_value() - other);
	}
	Uinteger::Value& Uinteger::Value::operator*=(UV other) {
		return operator=(unsigned_value() * other);
	}
	Uinteger::Value& Uinteger::Value::operator/=(UV other) {
		return operator=(unsigned_value() / other);
	}
	Uinteger::Value& Uinteger::Value::operator%=(UV other) {
		return operator=(unsigned_value() % other);
	}
	Uinteger::Value& Uinteger::Value::operator&=(UV other) {
		return operator=(unsigned_value() & other);
	}
	Uinteger::Value& Uinteger::Value::operator|=(UV other) {
		return operator=(unsigned_value() | other);
	}
	Uinteger::Value& Uinteger::Value::operator^=(UV other) {
		return operator=(unsigned_value() ^ other);
	}

	Uinteger::Value& Uinteger::Value::operator++() {
		sv_inc(get_SV(true));
		return *this;
	}
	Uinteger::Temp Uinteger::Value::operator++(int) {
		const UV ret = unsigned_value();
		++*this;
		return perl::Uinteger::Temp(interp, newSVuv(ret), true);
	}
	Uinteger::Value& Uinteger::Value::operator--() {
		sv_dec(get_SV(true));
		return *this;
	}
	Uinteger::Temp Uinteger::Value::operator--(int) {
		const UV ret = unsigned_value();
		--*this;
		return perl::Uinteger::Temp(interp, newSVuv(ret), true);
	}

	Uinteger::Value::operator UV() const {
		return unsigned_value();
	}
	UV Uinteger::Value::unsigned_value() const {
		return SvUV(get_SV(true));
	}
	bool Uinteger::Value::operator==(const perl::Uinteger::Value& right) const {
		return unsigned_value() == right.unsigned_value();
	}

	bool operator!=(const Uinteger::Value& left, const Uinteger::Value& right) {
		return !(left == right);
	}
	SV* Uinteger::Value::copy(const Scalar::Base& orig) {
		interpreter* interp = orig.interp;
		return newSVuv(orig.int_value());
	}
	bool Uinteger::Value::is_compatible_type(const Scalar::Base& val) {
		return SvUOK(val.get_SV(false));
	}
	const std::string& Uinteger::Value::cast_error() {
		static const std::string message("Not an unsigned Integer");
		return message;
	}

	/*
	 * Class Number::value
	 */

	Number::Value::Number(interpreter* _interp, SV* _handle) : Scalar::Base(_interp, _handle) {
	}
	Number::Value& Number::Value::operator=(const perl::Number::Value& other) {
		sv_setnv_mg(get_SV(false), other.number_value());
		return *this;
	}
	Number::Value& Number::Value::operator=(double other) {
		sv_setnv_mg(get_SV(false), other);
		return *this;
	}
	Number::Value& Number::Value::operator+=(double other) {
		return operator=(number_value() + other);
	}
	Number::Value& Number::Value::operator-=(double other) {
		return operator=(number_value() - other);
	}
	Number::Value& Number::Value::operator*=(double other) {
		return operator=(number_value() * other);
	}
	Number::Value& Number::Value::operator/=(double other) {
		return operator=(number_value() / other);
	}

	Number::Value::operator double() const {
		return number_value();
	}
	double Number::Value::number_value() const {
		return SvNV(get_SV(true));
	}

	SV* Number::Value::copy(const Scalar::Base& orig) {
		interpreter* interp = orig.interp;
		return newSVnv(orig.int_value());
	}
	bool Number::Value::is_compatible_type(const Scalar::Base& val) {
		return SvNOK(val.get_SV(false)) || Integer::is_compatible_type(val) || Uinteger::is_compatible_type(val);
	}
	const std::string& Number::Value::cast_error() {
		static const std::string message("Not a Number::Value");
		return message;
	}
	
	/*
	 * Class String::Value
	 */

	String::Value::String(interpreter* _interp, SV* _handle) : Scalar::Base(_interp, _handle) {
	}
	String::Value& String::Value::operator=(const perl::String::Value& other) {
		SvSetMagicSV(get_SV(false), other.get_SV(true));
		return *this;
	}
	String::Value& String::Value::operator=(const std::string& other) {
		sv_setpvn_mg(get_SV(false), other.c_str(), other.length());
		return *this;
	}
	String::Value& String::Value::operator=(Raw_string new_value) {
		sv_setpvn_mg(get_SV(false), new_value.value, new_value.length);
		return *this;
	}
	String::Value& String::Value::operator=(const char* new_value) {
		sv_setpv_mg(get_SV(false), new_value);
		return *this;
	}

	void String::Value::no_such_comparator() const {
	}
	String::Value::operator const Raw_string() const {
		return string_value();
	}
	const char* String::Value::get_raw() const {
		STRLEN len;
		return SvPVx(get_SV(true), len);
	}
//	String::Value::operator const char*() const {
//		return get_raw();
//	}
	const std::string String::Value::to_string() const {
		STRLEN len;
		const char* val = SvPVx(get_SV(true), len);
		return std::string(val, len);
	}	
	String::Value::operator String::bool_type() const {
		return length() > 0 ? &String::no_such_comparator : 0;
	}
	size_t String::Value::length() const {
		return sv_len(get_SV(true)); // Unicode!?
	}
	String::Value& String::Value::operator+=(const perl::String::Value& other) {
		sv_catsv_mg(get_SV(false), other.get_SV(true));
		return *this;
	}
	String::Value& String::Value::operator+=(const std::string& other) {
		sv_catpvn_mg(get_SV(false), other.c_str(), other.length());
		return *this;
	}
	String::Value& String::Value::operator+=(Raw_string other) {
		sv_catpvn_mg(get_SV(false), other.value, other.length);
		return *this;
	}
	String::Value& String::Value::operator+=(const char* other) {
		sv_catpv_mg(get_SV(false), other);
		return *this;
	}
	
	bool String::Value::operator==(const perl::String::Value& right) const {
		return sv_eq(get_SV(true), right.get_SV(true));
	}
	bool String::Value::operator==(const std::string& right) const {
		return strnEQ(get_raw(), right.c_str(), std::min(length(), right.length()) + 1);
	}
	bool String::Value::operator==(const char* right) const {
		return strnEQ(get_raw(), right, length() + 1);
	}

	namespace implementation {
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
		bool operator!=(const std::string& left, const perl::String& right) {
			return !(left == right);
		}

		bool operator!=(const String& left, const char* right) {
			return !(left == right);
		}
		bool operator!=(const char* left, const String& right) {
			return !(left == right);
		}
	}

	void String::Value::replace(size_t offset, size_t sublength, const char* other, size_t other_length) {
		sv_insert(get_SV(true), offset, sublength, const_cast<char*>(other), other_length);
	}
	void String::Value::replace(size_t offset, size_t sublength, Raw_string other) {
		replace(offset, sublength, other.value, other.length);
	}
	void String::Value::insert(size_t offset, Raw_string other) {
		replace(offset, 0, other.value, other.length);
	}
	void String::Value::insert(size_t offset, const char* other, size_t other_length) {
		replace(offset, 0, other, other_length);
	}

	const Array::Temp String::Value::unpack(const Raw_string pattern) const {
		return implementation::Call_stack(interp).unpack(pattern, *this);
	}
	Package String::Value::get_package(bool create) const {
		return Package(interp, get_SV(true), create);
	}

	bool String::Value::match(const perl::Regex& regex) const {
		return regex.match(*this);
	}
	bool String::Value::substitute(const perl::Regex& regex, const perl::String::Value& replacement) {
		return regex.substitute(*this, replacement);
	}
	bool String::Value::substitute(const perl::Regex& regex, Raw_string replacement) {
		return regex.substitute(*this, replacement);
	}

	SV* String::Value::copy(const Scalar::Base& other) {
		interpreter* const interp = other.interp;
		STRLEN len;
		const char* tmp = SvPVx(other.get_SV(true), len);
		SV* ret = newSVpvn(tmp, len);
		if (SvUTF8(other.get_SV(false))) {
			SvUTF8_on(ret);
		}
		return ret;
	}
	bool String::Value::is_compatible_type(const Scalar::Base& val) {
		return SvPOK(val.get_SV(false)) || Number::is_compatible_type(val);
	}
	const std::string& String::Value::cast_error() {
		static const std::string message("Not a String");
		return message;
	}
}
