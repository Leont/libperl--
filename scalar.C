#include "internal.h"
#include "perl++.h"
#define sv_2pv_flags(a,b,c) Perl_sv_2pv_flags(val.interp, a,b,c)
#define sv_setsv_flags(a,b,c)   Perl_sv_setsv_flags(aTHX_ a,b,c)
#define mg_set(a)       Perl_mg_set(aTHX_ a)
#define sv_2bool(a)     Perl_sv_2bool(aTHX_ a)

#undef SvIV
#undef SvUV
#undef SvNV
#undef SvTAINTED
#define SvIV(interp, sv) (SvIOK(sv) ? SvIVX(sv) : Perl_sv_2iv(interp, sv))
#define SvUV(interp, sv) (SvIOK(sv) ? SvUVX(sv) : Perl_sv_2uv(interp, sv))
#define SvNV(interp, sv) (SvNOK(sv) ? SvNVX(sv) : Perl_sv_2nv(interp, sv))
#define SvTAINTED(interp, sv)     (SvMAGICAL(sv) && Perl_sv_tainted(interp, sv)) 

namespace {
	int int_value(const perl::Scalar::Base& val) {
		return SvIV(val.interp, val.get_SV(true));
	}
	unsigned uint_value(const perl::Scalar::Base& val) {
		return SvUV(val.interp, val.get_SV(true));
	}
	double number_value(const perl::Scalar::Base& val) {
		return SvNV(val.interp, val.get_SV(true));
	}
	perl::Raw_string string_value(const perl::Scalar::Base& val) {
		unsigned len;
		const char* const tmp = SvPV(val.get_SV(true), len);
		return perl::Raw_string(tmp, len, SvUTF8(val.get_SV(false)));
	}
}

namespace perl {
	namespace implementation {
		/*
		 * Class Scalar::Base
		 */
		namespace scalar {
			Base::Base(interpreter* _interp, SV* _handle) : interp(_interp), handle(_handle) {
			}

			bool Base::is_tainted() const {
				return SvTAINTED(interp, handle);
			}
			void Base::taint() {
				Perl_sv_taint(interp, handle);
			}

			SV* Base::get_SV(bool do_magic) const {
				if (do_magic) {
					SvGETMAGIC(handle);
				}
				return handle;
			}
			bool Base::is_compatible_type(const Base&) {
				return true;
			}

			SV* Base::copy_sv(const Base& other) {
				return Perl_newSVsv(other.interp, other.get_SV(true));
			}
			const std::string& Base::cast_error() {
				static const std::string message("Not a scalar");
				return message;
			}
			void Base::tie_to(const Base& tier) {
				Perl_sv_magic(interp, get_SV(false), tier.get_SV(false), PERL_MAGIC_tiedscalar, "", 0);
			}
			void Base::untie() {
				if (MAGIC* mg = SvRMAGICAL(get_SV(false)) ? Perl_mg_find(interp, get_SV(false), PERL_MAGIC_tiedscalar) : NULL) {
					Ref<Any> tier = mg->mg_obj != NULL ?  Ref<Any>::Temp(interp, SvREFCNT_inc(reinterpret_cast<SV*>(mg->mg_obj)), true) : take_ref();
					if (tier.can("UNTIE")) {
						tier.call("UNTIE", static_cast<int>(SvREFCNT(SvRV(tier.get_SV(false)))));
					}
				}
				Perl_sv_unmagic(interp, get_SV(false), PERL_MAGIC_tiedscalar);
			}
			const Scalar::Temp Base::tied() const {
				if (MAGIC* mg = SvRMAGICAL(get_SV(false)) ? Perl_mg_find(interp, get_SV(false), PERL_MAGIC_tiedscalar) : NULL) {
					return (mg->mg_obj != NULL) ?  Scalar::Temp(interp, SvREFCNT_inc(reinterpret_cast<SV*>(mg->mg_obj)), true) : Scalar::Temp(take_ref());
				}
				return Scalar::Temp(interp, Perl_newSV(interp, 0), true);
			}
			
			const Ref<Any>::Temp Base::take_ref() const {
				return Ref<Any>::Temp(interp, Perl_newRV(interp, get_SV(false)), true);
			}
		}
	}
	std::ostream& operator<<(std::ostream& stream, const Scalar::Base& val) {
/*
 * Premature optimization...
		SV* handle = val.get_SV(true);
		if (SvPOK(handle)) {
			return stream << string_value(val);
		}
		else if (SvIOK(handle)) {
			return stream << SvIV(handle);
		}
		else if (SvUOK(handle)) {
			return stream << SvUV(handle);
		}
		else {
*/
			return stream << string_value(val);
//		}
	}

	namespace implementation {
		namespace helper {
			void decrement(const Scalar::Base& variable) {
				interpreter* const interp = variable.interp;
				if (SvOK(variable.get_SV(true))) {
					Perl_sv_free(interp, variable.get_SV(false));
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
				return Perl_newRV(interp, var.get_SV(false));
			}
		}
	}

	/*
	 * Class Scalar
	 */

	Scalar::Scalar(const Scalar& other) : Parent(other) {
	}
	Scalar::Scalar(const Scalar::Base& other) : Parent(other, override()) {
	}
	Scalar::Scalar(const Scalar::Temp& other) : Parent(other) {
	}

	bool Scalar::is_storage_type(const Any::Temp& ref) {
		return SvOK(reinterpret_cast<SV*>(ref.handle)) || implementation::is_this_type(ref, SVt_NULL);
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
	Scalar::Value& Scalar::Value::operator=(int new_value) {
		Perl_sv_setiv_mg(interp, get_SV(false), new_value);
		return *this;
	}
	Scalar::Value& Scalar::Value::operator=(unsigned new_value) {
		Perl_sv_setuv_mg(interp, get_SV(false), new_value);
		return *this;
	}
	Scalar::Value& Scalar::Value::operator=(double new_value) {
		Perl_sv_setnv_mg(interp, get_SV(false), new_value);
		return *this;
	}
	Scalar::Value& Scalar::Value::operator=(const char* new_value) {
		Perl_sv_setpvn_mg(interp, get_SV(false), new_value, strlen(new_value));
		return *this;
	}
	Scalar::Value& Scalar::Value::operator=(Raw_string new_value) {
		Perl_sv_setpvn_mg(interp, get_SV(false), new_value.value, new_value.length);
		return *this;
	}
	Scalar::Value& Scalar::Value::operator=(const std::string& new_value) {
		Perl_sv_setpvn_mg(interp, get_SV(false), new_value.c_str(), new_value.length());
		return *this;
	}

	bool Scalar::Value::as_bool() const {
		return SvTRUE(get_SV(true));
	}
	Scalar::Value::operator bool() const {
		return as_bool();
	}
	int Scalar::Value::int_value() const {
		return ::int_value(*this);
	}
	unsigned Scalar::Value::uint_value() const {
		return ::uint_value(*this);
	}
	double Scalar::Value::number_value() const {
		return ::number_value(*this);
	}
	Raw_string Scalar::Value::string_value() const {
		return ::string_value(*this);
	}

	Scalar::Value::operator int() const {
		return int_value();
	}

	Scalar::Value::operator unsigned() const {
		return uint_value();
	}
	Scalar::Value::operator double() const {
		return number_value();
	}
	Scalar::Value::operator Raw_string() const {
		return string_value();
	}
	Scalar::Value::operator const char*() const {
		return operator Raw_string();
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
		return Perl_sv_isobject(interp, get_SV(true));
	}
	bool Scalar::Value::is_exactly(const char* classname) const {
		return Perl_sv_isa(interp, get_SV(true), classname);
	}
	bool Scalar::Value::isa(const char* classname) const {
		return Perl_sv_derived_from(interp, get_SV(true), classname);
	}
	const char* Scalar::Value::get_classname() const {
		return HvNAME(SvSTASH(SvRV(get_SV(true))));
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
