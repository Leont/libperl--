#include "internal.h"
#include "XSUB.h"
#include "perl++.h"

#define stack_grow(a,b,c)  Perl_stack_grow(aTHX_ a,b,c)
#define markstack_grow() Perl_markstack_grow(aTHX)
#define push_scope()        Perl_push_scope(aTHX)
#define save_int(a)     Perl_save_int(aTHX_ a)
#define free_tmps()     Perl_free_tmps(aTHX)
#define pop_scope()     Perl_pop_scope(aTHX)
#define sv_2bool(a)        Perl_sv_2bool(aTHX_ a)
#define sv_2pv_flags(a,b,c) Perl_sv_2pv_flags(aTHX_ a,b,c)

#ifndef FLAG_UNPACK_DO_UTF8
// perl fails to export unpack flags. This is not very future-proof, but it works.
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

	const Ref<Code>::Temp take_ref(const Code::Value& var) {
		interpreter* const interp = var.interp;
		return Ref<Code>::Temp(interp, Perl_newRV(interp, reinterpret_cast<SV*>(var.handle)), true);
	}

	bool Code::is_storage_type(const Any::Temp& value) {
		return implementation::is_this_type(value, SVt_PVCV);
	}

	/*
	 * Class Perl_stack
	 */
	namespace implementation {
		Perl_stack::Perl_stack(interpreter* _interp) : interp(_interp), sp(PL_stack_sp) {
		}

		void Perl_stack::push(int value) {
			SV* const tmp = Perl_sv_2mortal(interp, Perl_newSViv(interp, value));
			SvREADONLY_on(tmp);
			XPUSHs(tmp);
		}
		void Perl_stack::push(unsigned value) {
			SV* const tmp = Perl_sv_2mortal(interp, Perl_newSVuv(interp, value));
			SvREADONLY_on(tmp);
			XPUSHs(tmp);
		}
		void Perl_stack::push(double value) {
			SV* const tmp = Perl_sv_2mortal(interp, Perl_newSVnv(interp, value));
			SvREADONLY_on(tmp);
			XPUSHs(tmp);
		}
		void Perl_stack::push(Raw_string value) {
			SV* const tmp = Perl_sv_2mortal(interp, Perl_newSVpvn(interp, value.value, value.length));
			SvREADONLY_on(tmp);
			XPUSHs(tmp);
		}
		void Perl_stack::push(const char* value) {
			SV* const tmp = Perl_sv_2mortal(interp, Perl_newSVpv(interp, value, 0));
			SvREADONLY_on(tmp);
			XPUSHs(tmp);
		}
		void Perl_stack::push(const std::string& string) {
			SV* const tmp = Perl_sv_2mortal(interp, Perl_newSVpvn(interp, string.c_str(), string.length()));
			SvREADONLY_on(tmp);
			XPUSHs(tmp);
		}

		void Perl_stack::push(const Scalar::Base& value) {
			XPUSHs(Perl_sv_2mortal(interp, SvREFCNT_inc(value.get_SV(false))));
		}
		void Perl_stack::push(const Scalar::Temp& value) {
			XPUSHs(Perl_sv_2mortal(interp, value.has_ownership() ? value.release() : SvREFCNT_inc(value.get_SV(false))));
		}
		void Perl_stack::push(const Array::Value& array) {
			const unsigned length = array.length();
			for(unsigned i = 0; i < length; i++) {
				push(array[i]);
			}
		}
		void Perl_stack::push(const null_type&) {
			//NOOP
		}

		/*
		 *  Class Call_stack :
		 *  the clean methods 
		 */
		
		const Scalar::Temp Call_stack::method_scalar(const char* const name) {
			assertion<Runtime_exception>( call_method(name, G_SCALAR) == 1, "More than one value returned in scalar call");
			return Scalar::Temp(interp, pop(), true);
		}
		const Array::Temp Call_stack::method_array(const char* name) {
			const int count = call_method(name, G_ARRAY);
			return Array::Temp(interp, pop_array(count), true);
		}

		const Scalar::Temp Call_stack::sub_scalar(const char* const name) {
			assertion<Runtime_exception>( call_sub(name, G_SCALAR) == 1, "More than one value returned in scalar call");
			return Scalar::Temp(interp, pop(), true);
		}
		const Scalar::Temp Call_stack::sub_scalar(const Ref<Code>::Value& ref) {
			assertion<Runtime_exception>( call_sub(ref.get_SV(true), G_SCALAR) == 1, "More than one value returned in scalar call");
			return Scalar::Temp(interp, pop(), true);
		}

		const Array::Temp Call_stack::sub_array(const char* const name) {
			const int count = call_sub(name, G_ARRAY);
			return Array::Temp(interp, pop_array(count), true);
		}
		const Array::Temp Call_stack::sub_array(const Ref<Code>::Value& ref) {
			const int count = call_sub(ref.get_SV(true), G_ARRAY);
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
		void Call_stack::finish_call(int count, intptr_t flags) {
			SPAGAIN;
	        if (SvTRUE(ERRSV)) {
				unwind_stack(count);

				STRLEN len;
				const char* mesg = SvPV(ERRSV, len);
				throw Runtime_exception(mesg, len);
			}
		}

		/* End of unclean methods */

		void Call_stack::unwind_stack(int count) {
			SP -= count;
			int ax = (SP - PL_stack_base) + 1;
			for(int i = 0; i < count; ++i) {
				Perl_sv_free(interp, ST(i));
			}
		}
		AV* Call_stack::pop_array(int count) {
			AV* ret = Perl_av_make(interp, count, SP - count + 1);
			unwind_stack(count);
			return ret;
		}

		int Call_stack::call_sub(const char* name, intptr_t flags) {
			prepare_call();
			const int ret = Perl_call_pv(interp, name, flags|G_EVAL);
			finish_call(ret, flags);
			return ret;
		}
		int Call_stack::call_sub(SV* ref, intptr_t flags) {
			prepare_call();
			const int ret = Perl_call_sv(interp, ref, flags|G_EVAL);
			finish_call(ret, flags);
			return ret;
		}
		int Call_stack::call_method(const char* name, intptr_t flags) {
			prepare_call();
			const int ret = Perl_call_method(interp, name, flags);
			finish_call(ret, flags);
			return ret;
		}

		const perl::String::Temp Call_stack::pack(const Raw_string pattern) {
			SV* ret = Perl_newSV(interp, 1);
			SV** base = PL_stack_base + TOPMARK + 1;
			Perl_packlist(interp, ret, const_cast<char*>(pattern.value), const_cast<char*>(pattern.value + pattern.length), base, SP + 1);
			return perl::String::Temp(interp, ret, true);
		}
		const Array::Temp Call_stack::unpack(const Raw_string pattern, const Raw_string value) {
			prepare_call();
			int count = Perl_unpackstring(interp, const_cast<char*>(pattern.value), const_cast<char*>(pattern.value + pattern.length), const_cast<char*>(value.value), const_cast<char*>(value.value + value.length), value.utf8 && !IN_BYTES ? FLAG_UNPACK_DO_UTF8 : 0);
			finish_call(count, G_ARRAY);
			return Array::Temp(interp, Perl_av_make(interp, count, SP - count + 1), true);
		}
	}
}
