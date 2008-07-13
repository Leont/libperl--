#include "internal.h"
#include "perl++.h"

#define sv_free(a) Perl_sv_free(aTHX_ a)

#if 1
namespace perl {
	Glob::Glob(const Glob& other) : interp(other.interp), value(other.value) {
		SvREFCNT_inc(reinterpret_cast<const SV*>(value));
	}
	Glob::~Glob() {
		SvREFCNT_dec(reinterpret_cast<const SV*>(value));
	}
//	Glob& Glob::operator=(const Glob& other);

	//TODO fix refcounting?
//	Glob& Glob::operator=(const scalar::value& other) {
//		//This is all guesswork!
//		if (GvSV(value) == NULL) {
//			GvSV(value) = newSV(0);
//		}
//		SvSetSV(GvSV(value), other.get_SV(true));
//		return *this;
//	}
//	Glob& Glob::operator=(const array& other) {
//		GvAV(value) = other.value;
//	}
//	Glob& Glob::operator=(const Array::Rvalue&);
//	Glob& Glob::operator=(const Hash&);
//	Glob& Glob::operator=(const Hash::Rvalue&);
//	Glob& Glob::operator=(const Code::Rvalue&);
	Raw_string Glob::name() const {
		return Raw_string(GvNAME(value), GvNAMELEN(value), true);
	}
	const Scalar::Temp Glob::scalar_value() const {
		return Scalar::Temp(interp, GvSV(value), false);
	}
	Array::Temp Glob::array_value() const {
		return Array::Temp(interp, GvAV(value), false);
	}
	Hash::Temp Glob::hash_value() const {
		return Hash::Temp(interp, GvHV(value), false);
	}
	const Code::Value Glob::code_value() const {
		return Code::Value(interp, GvCV(value));
	}
}
#endif
