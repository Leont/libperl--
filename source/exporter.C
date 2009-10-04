#include "internal.h"
#include <XSUB.h>
#include "perl++.h"

namespace perl {
	namespace implementation {
		void* get_magic_ptr(const MAGIC* magic) {
			return magic->mg_ptr;
		}
		void* get_magic_ptr(interpreter* interp, SV* var, int min_length) {
			const MAGIC* magic = mg_find(var, PERL_MAGIC_ext);
			if (magic == NULL || magic->mg_ptr == NULL || ( magic->mg_len < min_length && magic->mg_len != 0 ) ) {
				throw Runtime_exception("Magic error");
			}
			return magic->mg_ptr;
		}
		const Raw_string get_magic_string(interpreter* interp, SV* var) {
			const MAGIC* tmp = mg_find(var, PERL_MAGIC_ext);
			return tmp != NULL ? Raw_string(tmp->mg_ptr, tmp->mg_len, false) : Raw_string(NULL, 0, false);
		}
		bool has_magic_string(interpreter* interp, SV* var) {
			return mg_find(var, PERL_MAGIC_ext) != NULL;
		}
		bool has_magic_string(const Scalar::Base& var) {
			interpreter* const interp = var.interp;
			return mg_find(var.get_SV(false), PERL_MAGIC_ext) != NULL;
		}
		void set_magic_string(interpreter* interp, SV* var, Raw_string string) {
			sv_magic(var, NULL, PERL_MAGIC_ext, string.value, string.length);
		}
		void set_magic_string(interpreter* interp, SV* var, const void* value, unsigned length) {
			sv_magic(var, NULL, PERL_MAGIC_ext, static_cast<const char*>(value), length);
		}

		SV* make_magic_object(interpreter* interp, const void* obj, const Class_state& state, bool owns) {
			SV* const referee = state.use_hash ? reinterpret_cast<SV*>(newHV()) : newSV(0);
			Object_buffer buffer(const_cast<void*>(obj), state.family, owns);
			sv_magicext(referee, NULL, PERL_MAGIC_ext, state.magic_table, reinterpret_cast<const char*>(&buffer), sizeof buffer);
			SV* const ret = newRV_noinc(referee);
			return sv_bless(ret, gv_stashpv(state.classname, true));
		}
		void* get_magic_object_impl(interpreter* interp, SV* var, int min_length) {
			MAGIC* tmp = mg_find(SvRV(var), PERL_MAGIC_ext);
			if (tmp == NULL || tmp->mg_ptr == NULL || tmp->mg_len < min_length) {
				throw Not_an_object_exception();
			}
			return tmp->mg_ptr;
		}

		static MGVTBL* get_mgvtbl(magic_fun get_val, magic_fun set_val) {
			typedef std::pair<magic_fun, magic_fun> func_pair;
			static boost::ptr_map<func_pair, MGVTBL> magic_cache;
			func_pair key(get_val, set_val);
			if (magic_cache.find(key) == magic_cache.end()) {
				MGVTBL value = {get_val, set_val, 0, 0, 0 MAGIC_TAIL };
				magic_cache.insert(key, new MGVTBL(value));
			}
			return &magic_cache[key];
		}

		void attach_getset_magic(interpreter* interp, SV* var, magic_fun get_val, magic_fun set_val, const void* buffer, size_t buffer_length) {
			sv_magicext(var, NULL, PERL_MAGIC_uvar, get_mgvtbl(get_val, set_val), static_cast<const char*>(buffer), buffer_length);
		}


		Perl_mark::Perl_mark(int _ax, SV** _mark, unsigned _items) : ax(_ax), mark(_mark), items(_items) {
		}

		const Code::Value export_as(interpreter* interp, const char* name, void (*func)(interpreter* , CV*), const void* buffer, int length) {
			static char nothing[] = "";
			CV* const tmp = newXS(const_cast<char *>(name), func, nothing);
			implementation::set_magic_string(interp, reinterpret_cast<SV*>(tmp), static_cast<const char*>(buffer), length);
			return Code::Value(interp, tmp);
		}
		void die(interpreter* interp, const char* message) {
			Perl_croak(interp, "%s\n", message);
		}

		static boost::ptr_map<const std::type_info*, Class_state> typemap; //FIXME should be interpreter specific

		Class_state& register_type(interpreter*, const char* classname, const std::type_info& type, MGVTBL* magic_table, bool is_persistent, bool use_hash) {
			const std::type_info* key = &type;
			if (typemap.find(key) != typemap.end()) {
				if (typemap.at(key).magic_table != magic_table) {
					return typemap.at(key);
				}
				else {
					throw Runtime_exception("Can't register type over another type");
				}
			}
			else {
				Class_state* ret = new Class_state(classname, type, magic_table, is_persistent, use_hash);
				typemap.insert(key, ret);
				return *ret;
			}
		}

		static const char cache_namespace[] = "perl++/objects/";

		static SV* store_in_cache_impl(interpreter* interp, const void* address, const implementation::Class_state& state, bool owns) {
			std::string key(cache_namespace);
			key.append(reinterpret_cast<const char*>(&address), sizeof address);

			SV* const ret = make_magic_object(interp, address, state, owns);
			SV* const * const other = hv_store(PL_modglobal, key.data(), key.length(), newSVsv(ret), 0);
			if (!state.is_persistent) {
				sv_rvweaken(*other);
			}
			return ret;
		}
		Ref<Any>::Temp store_in_cache(interpreter* interp, const void* address, const implementation::Class_state& state) {
			return Ref<Any>::Temp(interp, store_in_cache_impl(interp, address, state, true), true);
		}

		SV* value_of_pointer(interpreter* interp, const void* address, const std::type_info& info) {
			std::string key(cache_namespace);
			key.append(reinterpret_cast<const char*>(&address), sizeof address);
			SV** entry = hv_fetch(PL_modglobal, key.data(), key.length(), 0);
			if (entry == NULL || !SvROK(*entry)) {
				return store_in_cache_impl(interp, address, typemap.at(&info), false);
			}
			return *entry;
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
	unsigned Argument_stack::get_num_args() const {
		return marker.items;
	}
	const Scalar::Temp Argument_stack::operator[](unsigned pos) const {
		assertion<Runtime_exception>(pos < marker.items, "No such argument!");
		return Scalar::Temp(interp, ST(pos), false);
	}
	Scalar::Temp Argument_stack::operator[](unsigned pos) {
		assertion<Runtime_exception>(pos < marker.items, "No such argument!");
		return Scalar::Temp(interp, ST(pos), false);
	}
	context Argument_stack::get_context() const {
		return GIMME_V == G_VOID  ? VOID : 
			   GIMME_V == G_ARRAY ? LIST : 
			   SCALAR;
	}
}
