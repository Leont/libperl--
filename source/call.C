#include "internal.h"
#include "perl++.h"
#include "regex_impl.h"

#ifndef FLAG_UNPACK_DO_UTF8
// XXX: perl fails to export unpack flags. This is not very future-proof, but it works.
#define FLAG_UNPACK_DO_UTF8   0x08
#endif

namespace perl {
	/*
	 * Class Code
	 */

	const std::string& Code::cast_error() {
		static const std::string message("Not a subroutine");
		return message;
	}

	/*
	 * Class Code::Rvalue
	 */

	Code::Value::Value(interpreter* _interp, CV* _handle) : interp(_interp), handle(_handle) {
	}

	const Ref<Code>::Temp Code::Value::take_ref() const {
		return Ref<Code>::Temp(interp, newRV_inc(reinterpret_cast<SV*>(handle)), true);
	}

	bool Code::is_storage_type(const Any::Temp& value) {
		return implementation::is_this_type(value, SVt_PVCV);
	}

	namespace implementation {
		/*
		 * Class Perl_stack
		 */
		Perl_stack::Perl_stack(interpreter* _interp) : interp(_interp), sp(PL_stack_sp) {
		}

		void Perl_stack::push(IV value) {
			SV* const tmp = sv_2mortal(newSViv(value));
			SvREADONLY_on(tmp);
			XPUSHs(tmp);
		}
		void Perl_stack::push(UV value) {
			SV* const tmp = sv_2mortal(newSVuv(value));
			SvREADONLY_on(tmp);
			XPUSHs(tmp);
		}
		void Perl_stack::push(NV value) {
			SV* const tmp = sv_2mortal(newSVnv(value));
			SvREADONLY_on(tmp);
			XPUSHs(tmp);
		}
		void Perl_stack::push(Raw_string value) {
			SV* const tmp = sv_2mortal(newSVpvn(value.value, value.length));
			SvREADONLY_on(tmp);
			XPUSHs(tmp);
		}
		void Perl_stack::push(const char* value) {
			SV* const tmp = sv_2mortal(newSVpv(value, 0));
			SvREADONLY_on(tmp);
			XPUSHs(tmp);
		}
		void Perl_stack::push(const std::string& string) {
			SV* const tmp = sv_2mortal(newSVpvn(string.c_str(), string.length()));
			SvREADONLY_on(tmp);
			XPUSHs(tmp);
		}

		void Perl_stack::push(const Scalar::Base& value) {
			XPUSHs(sv_2mortal(SvREFCNT_inc(value.get_SV(false))));
		}
		void Perl_stack::push(const Scalar::Temp& value) {
			XPUSHs(sv_2mortal(value.has_ownership() ? value.release() : SvREFCNT_inc(value.get_SV(false))));
		}
		void Perl_stack::push(const Array::Value& array) {
			const unsigned length = array.length();
			for(unsigned i = 0; i < length; i++) {
				push(array[i]);
			}
		}
		void Perl_stack::push(const Regex& regex) {
			XPUSHs(sv_2mortal(SvREFCNT_inc(regex.get_SV())));
		}
		void Perl_stack::push(const null_type&) {
			//NOOP
		}

		/*
		 *  Class Call_stack :
		 *  the clean methods 
		 */
		
		const Scalar::Temp Call_stack::method_scalar(const char* const name) {
			assertion<Runtime_exception>( method_call(name, G_SCALAR) == 1, "More than one value returned in scalar call");
			return Scalar::Temp(interp, pop(), true);
		}
		const Array::Temp Call_stack::method_list(const char* name) {
			const int count = method_call(name, G_ARRAY);
			return Array::Temp(interp, pop_array(count), true);
		}

		const Scalar::Temp Call_stack::sub_scalar(const char* const name) {
			assertion<Runtime_exception>( sub_call(name, G_SCALAR) == 1, "More than one value returned in scalar call");
			return Scalar::Temp(interp, pop(), true);
		}
		const Scalar::Temp Call_stack::sub_scalar(const Ref<Code>::Value& ref) {
			assertion<Runtime_exception>( sub_call(ref.get_SV(true), G_SCALAR) == 1, "More than one value returned in scalar call");
			return Scalar::Temp(interp, pop(), true);
		}
		const Scalar::Temp Call_stack::sub_scalar(const Scalar::Value& ref) {
			assertion<Runtime_exception>( sub_call(ref.get_SV(true), G_SCALAR) == 1, "More than one value returned in scalar call");
			return Scalar::Temp(interp, pop(), true);
		}

		const Array::Temp Call_stack::sub_list(const char* const name) {
			const int count = sub_call(name, G_ARRAY);
			return Array::Temp(interp, pop_array(count), true);
		}
		const Array::Temp Call_stack::sub_list(const Ref<Code>::Value& ref) {
			const int count = sub_call(ref.get_SV(true), G_ARRAY);
			return Array::Temp(interp, pop_array(count), true);
		}
		const Array::Temp Call_stack::sub_list(const Scalar::Value& ref) {
			const int count = sub_call(ref.get_SV(true), G_ARRAY);
			return Array::Temp(interp, pop_array(count), true);
		}

		/* Unclean Call_stack methods, Here Be Dragons! */

		SV* Call_stack::pop() {
			return SvREFCNT_inc(POPs);
		}

		Call_stack::Call_stack(interpreter* _interp) : Perl_stack(_interp) {
			ENTER;
			SAVETMPS;
			PUSHMARK(SP);
		}
		
		Call_stack::~Call_stack() {
			PUTBACK;
			FREETMPS;
			LEAVE;
		}

		void Call_stack::prepare_call() {
			PUTBACK;
		}
		void Call_stack::finish_call() {
			SPAGAIN;
	        if (SvTRUE(ERRSV)) {
				POPs;
				STRLEN len;
				const char* mesg = SvPV(ERRSV, len);
				throw Runtime_exception(mesg, len);
			}
		}

		/* End of unclean methods */

		AV* Call_stack::pop_array(int count) {
			AV* ret = av_make(count, SP - count + 1);
			SP -= count;
			return ret;
		}

		int Call_stack::sub_call(const char* name, intptr_t flags) {
			prepare_call();
			const int ret = call_pv(name, flags|G_EVAL);
			finish_call();
			return ret;
		}
		int Call_stack::sub_call(SV* ref, intptr_t flags) {
			prepare_call();
			const int ret = call_sv(ref, flags|G_EVAL);
			finish_call();
			return ret;
		}
		int Call_stack::method_call(const char* name, intptr_t flags) {
			prepare_call();
			const int ret = call_method(name, flags);
			finish_call();
			return ret;
		}

		const perl::String::Temp Call_stack::pack(const Raw_string pattern) {
			SV* ret = newSV(1);
			SV** base = PL_stack_base + TOPMARK + 1;
			packlist(ret, const_cast<char*>(pattern.value), const_cast<char*>(pattern.value + pattern.length), base, SP + 1);
			return perl::String::Temp(interp, ret, true);
		}
		const Array::Temp Call_stack::unpack(const Raw_string pattern, const Raw_string value) {
			prepare_call();
			int count = unpackstring(const_cast<char*>(pattern.value), const_cast<char*>(pattern.value + pattern.length), const_cast<char*>(value.value), const_cast<char*>(value.value + value.length), value.utf8 && !IN_BYTES ? FLAG_UNPACK_DO_UTF8 : 0);
			finish_call();
			return Array::Temp(interp, av_make(count, SP - count + 1), true);
		}

		const Scalar::Temp Call_stack::eval_scalar(SV* string) {
			eval_sv(string, G_SCALAR);
			finish_call();
			return Scalar::Temp(interp, pop(), true);
		}
		const Array::Temp Call_stack::eval_list(SV* string) {
			const int count = eval_sv(string, G_ARRAY);
			finish_call();
			return Array::Temp(interp, pop_array(count), true);
		}

		int Call_stack::match_scalar(REGEXP* rx, const Scalar::Base& string, IV flags) {
			match(rx, string.get_SV(true), G_SCALAR, flags);
			finish_call();
			return Scalar::Temp(interp, pop(), true);
		}
		const Array::Temp Call_stack::match_list(REGEXP* rx, const Scalar::Base& string, IV flags) {
			const int count = match(rx, string.get_SV(true), G_ARRAY, flags);
			finish_call();
			return Array::Temp(interp, pop_array(count), true);
		}

		int Call_stack::subst_scalar(REGEXP* rx, const Scalar::Base& string, const Scalar::Base& dest, IV flags) {
			subst(rx, string.get_SV(true), dest.get_SV(true), flags);
			finish_call();
			return Scalar::Temp(interp, pop(), true);
		}
		const Array::Temp Call_stack::subst_array(REGEXP* rx, const Scalar::Base& string, const Scalar::Base& dest, IV flags) {
			const int count = subst(rx, string.get_SV(true), dest.get_SV(true), flags);
			finish_call();
			return Array::Temp(interp, pop_array(count), true);
		}

		/*
		 * class Stash
		 */

		Stash::Stash(const Package& package) : interp(package.interp), stash(package.stash) {
		}
		Stash::Stash(const reference::Reference_base& value) : interp(value.interp), stash(SvSTASH(SvRV(value.get_SV(false)))) {
		}
		static inline HV* get_stash(const Scalar::Value& value) {
			SV* const handler = value.get_SV(true);
			interpreter* const interp = value.interp;
			return (SvROK(handler) && sv_isobject(handler)) ? SvSTASH(SvRV(handler)) : gv_stashsv(handler, false);
		}
		Stash::Stash(const Scalar::Value& value) : interp(value.interp), stash(get_stash(value)) {
		}
		bool Stash::can(Raw_string name) const {
			if (stash) {
				GV* gv = gv_fetchmeth_autoload(stash, name.value, name.length, -1);
				return gv && isGV(gv) && CvGV(gv);
			}
			return false;
		}
		const Ref<Code>::Temp Stash::get_method(Raw_string name) const {
			GV* const glob = gv_fetchmeth_autoload(stash, name.value, name.length, -1);
			if (glob == NULL || !isGV(glob) || CvGV(glob) == NULL) {
				throw Runtime_exception("method doesn't exist");//TODO No such method exception??
			}
			CV* const codeval = GvCV(glob);
			return Code::Value(interp, codeval).take_ref();
		}
	}
}
