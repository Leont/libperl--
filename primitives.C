#include "internal.h"
#include "perl++.h"

#define sv_2iv(a) Perl_sv_2iv(aTHX_ a)
#define sv_2uv(a) Perl_sv_2uv(aTHX_ a)
#define sv_2nv(a) Perl_sv_2nv(aTHX_ a)
#define sv_setsv_flags(a,b,c)   Perl_sv_setsv_flags(aTHX_ a,b,c)
#define mg_set(a)       Perl_mg_set(aTHX_ a)
#define sv_2pv_flags(a,b,c) Perl_sv_2pv_flags(aTHX_ a,b,c)

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
			Perl_sv_setpviv_mg(interp, get_SV(false), other.int_value());
			return *this;
		}
		Integer& Integer::operator=(int other) {
			Perl_sv_setpviv_mg(interp, get_SV(false), other);
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
			Perl_sv_inc(interp, get_SV(true));
			return *this;
		}
		Integer Integer::operator++(int) {
			const int ret = int_value();
			++*this;
			return Integer(interp, Perl_newSViv(interp, ret));
		}
		Integer& Integer::operator--() {
			Perl_sv_dec(interp, get_SV(true));
			return *this;
		}
		Integer Integer::operator--(int) {
			const int ret = int_value();
			--*this;
			return Integer(interp, Perl_newSViv(interp, ret));
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
			Perl_sv_setuv_mg(interp, get_SV(false), other.unsigned_value());
			return *this;
		}
		Uinteger& Uinteger::operator=(unsigned other) {
			Perl_sv_setuv_mg(interp, get_SV(false), other);
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
			Perl_sv_inc(interp, get_SV(true));
			return *this;
		}
		Uinteger Uinteger::operator++(int) {
			const unsigned ret = unsigned_value();
			++*this;
			return Uinteger(interp, Perl_newSVuv(interp, ret));
		}
		Uinteger& Uinteger::operator--() {
			Perl_sv_dec(interp, get_SV(true));
			return *this;
		}
		Uinteger Uinteger::operator--(int) {
			const unsigned ret = unsigned_value();
			--*this;
			return Uinteger(interp, Perl_newSVuv(interp, ret));
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
			Perl_sv_setnv_mg(interp, get_SV(false), other.number_value());
			return *this;
		}
		Number& Number::operator=(double other) {
			Perl_sv_setnv_mg(interp, get_SV(false), other);
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
			Perl_sv_setpvn_mg(interp, get_SV(false), other.c_str(), other.length());
			return *this;
		}
		String& String::operator=(Raw_string new_value) {
			Perl_sv_setpvn_mg(interp, get_SV(false), new_value.value, new_value.length);
			return *this;
		}
		String& String::operator=(const char* new_value) {
			Perl_sv_setpv_mg(interp, get_SV(false), new_value);
			return *this;
		}

		const Raw_string String::get_raw_string() const {
			STRLEN len;
			const char* ret = SvPV(get_SV(true), len);
			return Raw_string(ret, len, SvUTF8(get_SV(false)));//only once!
		}
		String::operator const Raw_string() const {
			return get_raw_string();
		}
		const char* String::get_raw() const {
			return get_raw_string();
		}
		String::operator const char*() const {
			STRLEN len;
			return SvPV(get_SV(true), len);
		}
		unsigned String::length() const {
			return Perl_sv_len(interp, get_SV(true)); // Unicode!?
		}
		String& String::operator+=(const String& other) {
			Perl_sv_catsv_mg(interp, get_SV(false), other.get_SV(true));
			return *this;
		}
		String& String::operator+=(const std::string& other) {
			Perl_sv_catpvn_mg(interp, get_SV(false), other.c_str(), other.length());
			return *this;
		}
		String& String::operator+=(Raw_string other) {
			Perl_sv_catpvn_mg(interp, get_SV(false), other.value, other.length);
			return *this;
		}
		String& String::operator+=(const char* other) {
			Perl_sv_catpv_mg(interp, get_SV(false), other);
			return *this;
		}
		
		bool String::operator==(const String& right) const {
			return Perl_sv_eq(interp, get_SV(true), right.get_SV(true));
		}
		bool String::operator==(const std::string& right) const {
			return *this == Raw_string(right);
		}
		
		bool operator==(const std::string& left, const String& right) {
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

		void String::replace(unsigned offset, unsigned sublength, const char* other, unsigned other_length) {
			Perl_sv_insert(interp, get_SV(true), offset, sublength, const_cast<char*>(other), other_length);
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

		SV* String::copy(const Scalar::Base& other) {
			interpreter* const interp = other.interp;
			STRLEN len;
			const char* tmp = SvPV(other.get_SV(true), len);
			SV* ret = Perl_newSVpvn(interp, tmp, len);
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
