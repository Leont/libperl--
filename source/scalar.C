#include "internal.h"
#include "perl++.h"

namespace perl {
	Scalar::Base::Base(interpreter* _interp, SV* _handle) : interp(_interp), handle(_handle) {
	}

	bool Scalar::Base::is_tainted() const {
		return SvTAINTED(handle);
	}
	void Scalar::Base::taint() {
		sv_taint(handle);
	}

	SV* Scalar::Base::get_SV(bool do_magic) const {
		if (do_magic) {
			SvGETMAGIC(handle);
		}
		return handle;
	}

	IV Scalar::Base::int_value() const {
		return SvIVx(get_SV(true));
	}
	UV Scalar::Base::uint_value() const {
		return SvUVx(get_SV(true));
	}
	double Scalar::Base::number_value() const {
		return SvNVx(get_SV(true));
	}
	perl::Raw_string Scalar::Base::string_value() const {
		size_t len;
		const char* const tmp = SvPVx(get_SV(true), len);
		return perl::Raw_string(tmp, len, SvUTF8(get_SV(false)));
	}

	void Scalar::Base::tie_to(const Scalar::Base& tier) {
		sv_magic(get_SV(false), tier.get_SV(false), PERL_MAGIC_tiedscalar, "", 0);
	}
	void Scalar::Base::untie() {
		typedef Temp_template< reference::Scalar_ref<Base> > Tier;
		if (MAGIC* mg = SvRMAGICAL(get_SV(false)) ? mg_find(get_SV(false), PERL_MAGIC_tiedscalar) : NULL) {
			Tier tier = mg->mg_obj != NULL ? Tier(interp, SvREFCNT_inc(mg->mg_obj), true) : take_ref();
			if (tier.can("UNTIE")) {
				tier.call("UNTIE", static_cast<int>(SvREFCNT(SvRV(tier.get_SV(false)))));
			}
		}
		sv_unmagic(get_SV(false), PERL_MAGIC_tiedscalar);
	}
	const Scalar::Temp Scalar::Base::tied() const {
		if (MAGIC* mg = SvRMAGICAL(get_SV(false)) ? mg_find(get_SV(false), PERL_MAGIC_tiedscalar) : NULL) {
			return (mg->mg_obj != NULL) ?  Scalar::Temp(interp, SvREFCNT_inc(mg->mg_obj), true) : Scalar::Temp(take_ref());
		}
		return Scalar::Temp(interp, newSV(0), true);
	}
	
	bool Scalar::Base::is_compatible_type(const Scalar::Base&) {
		return true;
	}
	SV* Scalar::Base::copy_sv(const Scalar::Base& other) {
		interpreter* const interp = other.interp;
		return newSVsv(other.get_SV(true));
	}
	const std::string& Scalar::Base::cast_error() {
		static const std::string message("Not a scalar");
		return message;
	}

	std::ostream& operator<<(std::ostream& stream, const Scalar::Base& val) {
		interpreter* interp = val.interp;
		SV* handle = val.get_SV(true);
		if (SvPOK(handle)) {
			return stream << val.string_value();//FIXME: double magic
		}
		else if (SvIOK(handle)) {
			return stream << SvIV(handle);
		}
		else if (SvUOK(handle)) {
			return stream << SvUV(handle);
		}
		else {
			return stream << val.string_value();
		}
	}

	namespace implementation {
		namespace helper {
			void decrement(const Scalar::Base& variable) {
				interpreter* const interp = variable.interp;
				if (SvOK(variable.get_SV(true))) {//XXX
					SvREFCNT_dec(variable.get_SV(false));
				}
			}
			void set_scalar(Scalar::Base& sink, const Scalar::Base& source) {
				interpreter* const interp = sink.interp;
				SvSetMagicSV(sink.get_SV(false), source.get_SV(true));
			}
			const Any::Temp dereference(const Scalar::Base& val) {
				interpreter* const interp = val.interp;
				if (!reference::Nonscalar<Any>::is_compatible_type(val)) {
					throw Cast_exception("Not a reference");
				}
				return Any::Temp(interp, SvRV(val.get_SV(true)));
			}
			void share(const Scalar::Base& var) {
				interpreter* const interp = var.interp;
				SvSHARE(var.get_SV(false));
			}
			SV* take_ref(const Scalar::Base& var) {
				interpreter* const interp = var.interp;
				return newRV(var.get_SV(false));
			}
			bool is_scalar_type(const Any::Temp& ref) {
				return SvOK(reinterpret_cast<SV*>(ref.handle)) || implementation::is_this_type(ref, SVt_NULL);
			}

		}
	}

	const Scalar::Temp convert(const Scalar::Base& val) {
		return Scalar::Temp(val.interp, val.get_SV(false), false);
	}

	/*
	 * Class Scalar::Value
	 */
	Scalar::Value::Value(interpreter* _interp, SV* _value) : Base(_interp, _value) {
	}

	Scalar::Value& Scalar::Value::operator=(const Scalar::Base& other) {
		SvSetMagicSV(get_SV(false), other.get_SV(true));
		return *this;
	}
	Scalar::Value& Scalar::Value::operator=(const Scalar::Value& other) {
		SvSetMagicSV(get_SV(false), other.get_SV(true));
		return *this;
	}
	Scalar::Value& Scalar::Value::operator=(IV new_value) {
		sv_setiv_mg(get_SV(false), new_value);
		return *this;
	}
	Scalar::Value& Scalar::Value::operator=(UV new_value) {
		sv_setuv_mg(get_SV(false), new_value);
		return *this;
	}
	Scalar::Value& Scalar::Value::operator=(double new_value) {
		sv_setnv_mg(get_SV(false), new_value);
		return *this;
	}
	Scalar::Value& Scalar::Value::operator=(const char* new_value) {
		sv_setpvn_mg(get_SV(false), new_value, strlen(new_value));
		return *this;
	}
	Scalar::Value& Scalar::Value::operator=(Raw_string new_value) {
		sv_setpvn_mg(get_SV(false), new_value.value, new_value.length);
		return *this;
	}
	Scalar::Value& Scalar::Value::operator=(const std::string& new_value) {
		sv_setpvn_mg(get_SV(false), new_value.c_str(), new_value.length());
		return *this;
	}

	Scalar::Value& Scalar::Value::operator++() {
		sv_inc(get_SV(true));
		return *this;
	}
	Scalar::Value& Scalar::Value::operator--() {
		sv_dec(get_SV(true));
		return *this;
	}

	namespace implementation {
		namespace scalar {
			bool operator==(const Scalar::Value& left, IV right) {
				return left.int_value() == right;
			}
			bool operator==(const Scalar::Value& left, UV right) {
				return left.uint_value() == right;
			}
			bool operator==(const Scalar::Value& left, const char* right) {
				return left.string_value() == right;
			}
			bool operator==(const Scalar::Value& left, Raw_string right) {
				return left.string_value() == right;
			}
			bool operator==(const Scalar::Value& left, const std::string& right) {
				return left.string_value() == right;
			}

			bool operator!=(const Value& left, const char* right) {
				return !(left == right);
			}
			bool operator!=(const Value& left, Raw_string right) {
				return !(left == right);
			}
			bool operator!=(const Value& left, const std::string& right) {
				return !(left == right);
			}
		}
	}

	bool Scalar::Value::as_bool() const {
		return SvTRUE(get_SV(true));
	}
	Scalar::Value::operator bool() const {
		return as_bool();
	}

	Scalar::Value::operator int() const {
		return int_value();
	}
	Scalar::Value::operator long() const {
		return int_value();
	}
	Scalar::Value::operator short() const {
		return int_value();
	}
	Scalar::Value::operator long long() const {
		return int_value();
	}
	Scalar::Value::operator unsigned int() const {
		return uint_value();
	}
	Scalar::Value::operator unsigned long() const {
		return uint_value();
	}
	Scalar::Value::operator unsigned short() const {
		return uint_value();
	}
	Scalar::Value::operator unsigned long long() const {
		return uint_value();
	}
	
	Scalar::Value::operator double() const {
		return number_value();
	}
	Scalar::Value::operator float() const {
		return number_value();
	}
	Scalar::Value::operator long double() const {
		return number_value();
	}
	Scalar::Value::operator Raw_string() const {
		return string_value();
	}
//	Scalar::Value::operator const char*() const {
//		return operator Raw_string();
//	}
	Scalar::Value::operator const std::string() const {
		size_t len;
		const char* const tmp = SvPVx(get_SV(true), len);
		return std::string(tmp, len);
	}

	bool Scalar::Value::defined() const {
		return SvOK(get_SV(true));
	}
	size_t Scalar::Value::length() const {
		return sv_len(get_SV(true));
	}

	const Array::Temp Scalar::Value::unpack(const Raw_string pattern) const {
		return implementation::Call_stack(interp).unpack(pattern, *this);
	}

	SV* Scalar::Value::copy(const Scalar::Base& other) {
		return copy_sv(other);
	}
	Scalar::Temp Scalar::Value::operator[](int index) const {
		const Any::Temp ret = helper::dereference(*this);
		if (!Array::is_storage_type(ret)) {
			throw Cast_exception(Ref<Array>::Value::cast_error());
		}
		Array::Temp array(ret.interp, reinterpret_cast<AV*>(ret.handle), false);
		return array[index];
	}
	Scalar::Temp Scalar::Value::operator[](Raw_string key) const {
		const Any::Temp ret = helper::dereference(*this);
		if (!Hash::is_storage_type(ret)) {
			throw Cast_exception(Ref<Hash>::cast_error());
		}
		Hash::Temp hash(ret.interp, reinterpret_cast<HV*>(ret.handle), false);
		return hash[key];
	}

	bool Scalar::Value::is_object() const {
		return sv_isobject(get_SV(true));
	}
	bool Scalar::Value::is_exactly(const char* classname) const {
		return sv_isa(get_SV(true), classname);
	}
	bool Scalar::Value::isa(const char* classname) const {
		return sv_derived_from(get_SV(true), classname);
	}
	const char* Scalar::Value::get_classname() const {
		return HvNAME(SvSTASH(SvRV(get_SV(true))));
	}
	void Scalar::Value::grow(size_t new_length) {
		SV* sv = get_SV(false);
		if (SvTYPE(sv) < SVt_PV)
			sv_upgrade(sv, SVt_PV);
		SvGROW(sv, new_length);
	}

	compared cmp(const Scalar::Base& left, const Scalar::Base& right) {
		interpreter* interp = left.interp;
		return static_cast<compared>(sv_cmp(left.get_SV(true), right.get_SV(true)));
	}
	bool eq(const Scalar::Base& left, const Scalar::Base& right) {
		interpreter* interp = left.interp;
		return sv_eq(left.get_SV(true), right.get_SV(true));
	}

	/*
	 * Class locker
	 */

	SV* lock::lock_SV(SV* value) {
		SvLOCK(value);
		return value;
	}
	lock::lock(const Array& _variable) : interp(_variable.interp), variable(lock_SV(reinterpret_cast<SV*>(_variable.handle))) {
	}
	lock::lock(const Hash& _variable) : interp(_variable.interp), variable(lock_SV(reinterpret_cast<SV*>(_variable.handle))) {
	}
	lock::~lock() {
		SvUNLOCK(variable);
	}

	/*
	 * Implementation details
	 */
	namespace implementation {
		bool is_this_type(const Any::Temp& var, unsigned int type) {
			return SvTYPE(reinterpret_cast<const SV*>(var.handle)) == type;
		}
	}
}
