#include <XSUB.h>
#include "internal.h"
#include "perl++.h"

namespace perl {
	namespace implementation {
		Perl_mark::Perl_mark(int _ax, SV** _mark, unsigned _items) : ax(_ax), mark(_mark), items(_items) {
		}

		const Code::Value export_as(interpreter* interp, const char* name, void (*func)(interpreter* , CV*), const void* buffer, int length) {
			static char nothing[] = "";
			CV* const tmp = newXS(const_cast<char *>(name), func, nothing);
			implementation::set_magic_string(interp, reinterpret_cast<SV*>(tmp), reinterpret_cast<const char*>(buffer), length);
			return Code::Value(interp, tmp);
		}
		void die(interpreter* interp, const char* message) {
			croak("%s\n", message);
		}

		static const char cache_namespace[] = "perl++/objects/";

		Ref<Any>::Temp get_from_cache(interpreter* interp, const void* address) {
			std::string key(cache_namespace);
			key.append(reinterpret_cast<const char*>(&address), sizeof(void*));
			if (! hv_exists(PL_modglobal, key.c_str(), key.length())) {
				throw No_such_object_exception();
			}
			SV* const* const ret = hv_fetch(PL_modglobal, key.c_str(), key.length(), 0);
			return Ref<Any>::Temp(interp, *ret, false);
		}
		Ref<Any>::Temp store_in_cache(interpreter* interp, void* address, const implementation::classes::State& state) {
			std::string key(cache_namespace);
			key.append(reinterpret_cast<const char*>(&address), sizeof(void*));

			SV* const ret = make_magic_object(interp, address, state);
			SV* const * const other = hv_store(PL_modglobal, key.c_str(), key.length(), newSVsv(ret), 0);
			if (!state.persistent) {
				sv_rvweaken(*other);
			}
			return Ref<Any>::Temp(interp, ret, true);
		}
	}

	namespace {
		implementation::Perl_mark make_mark(interpreter* interp) {
			dXSARGS;
			return implementation::Perl_mark(ax, mark, items);
		}
	}

	Argument_stack::Argument_stack(interpreter* _interp) : Perl_stack(_interp), marker(make_mark(interp)), return_num(0) {
	}
#define ax marker.ax
	Argument_stack::~Argument_stack() {
		XSRETURN(return_num);
	}
	void Argument_stack::pre_push() {
		XSprePUSH;
	}
	const Array::Temp Argument_stack::get_arg() const {
		return Array::Temp(interp, av_make(marker.items, sp - marker.items + 1), true);
	}
	Array::Temp Argument_stack::get_arg() {
		return Array::Temp(interp, av_make(marker.items, sp - marker.items + 1), true);
	}
	const Scalar::Temp Argument_stack::operator[](unsigned pos) const {
		assertion<Runtime_exception>(pos < marker.items, "No such argument!");
		return Scalar::Temp(interp, ST(pos), false);
	}
	Scalar::Temp Argument_stack::operator[](unsigned pos) {
		assertion<Runtime_exception>(pos < marker.items, "No such argument!");
		return Scalar::Temp(interp, ST(pos), false);
	}
	context Argument_stack::get_contest() const {
		return GIMME_V == G_VOID  ? VOID : 
			   GIMME_V == G_ARRAY ? LIST : 
			   SCALAR;
	}
}
