#include "internal.h"
#include "perl++.h"

namespace perl {
	namespace implementation {
		void* get_magic_ptr(const MAGIC* magic) {
			return magic->mg_ptr;
		}
		void* get_magic_ptr(interpreter* interp, SV* var, int min_length) {
			MAGIC* tmp = mg_find(var, PERL_MAGIC_ext);
			if (tmp == NULL || tmp->mg_ptr == NULL || tmp->mg_len < min_length) {
				throw Runtime_exception("Magic error");
			}
			return tmp->mg_ptr;
		}
		const Raw_string get_magic_string(interpreter* interp, SV* var) {
			MAGIC* tmp = mg_find(var, PERL_MAGIC_ext);
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
			sv_magic(var, NULL, PERL_MAGIC_ext, reinterpret_cast<const char*>(value), length);
		}

		SV* make_magic_object(interpreter* interp, void* obj, const classes::State& state) {
			SV* const referee = state.hash ? reinterpret_cast<SV*>(newHV()) : newSV(0);
			Object_buffer buffer(obj, true);
			sv_magicext(referee, NULL, PERL_MAGIC_ext, state.magic_table, reinterpret_cast<const char*>(&buffer), sizeof buffer);
			SV* const ret = newRV_noinc(referee);
			sv_bless(ret, gv_stashpv(state.classname, true));
			return ret;
		}
		void** get_magic_object_impl(interpreter* interp, SV* var, int min_length) {
			MAGIC* tmp = mg_find(SvRV(var), PERL_MAGIC_ext);
			if (tmp == NULL || tmp->mg_ptr == NULL || tmp->mg_len < min_length) {
				throw Not_an_object_exception();
			}
			return reinterpret_cast<void**>(tmp->mg_ptr);
		}

		static MGVTBL* get_mgvtbl(magic_fun get_val, magic_fun set_val) {
			typedef std::pair<magic_fun, magic_fun> func_pair;
			static boost::ptr_map<func_pair, MGVTBL> magic_cache;
			func_pair key(get_val, set_val);
			if (magic_cache.find(key) == magic_cache.end()) {
				MGVTBL value = {get_val, set_val};
				magic_cache.insert(key, new MGVTBL(value));
			}
			return &magic_cache[key];
		}

		void attach_getset_magic(interpreter* interp, SV* var, magic_fun get_val, magic_fun set_val, const void* buffer, size_t buffer_length) {
			sv_magicext(var, NULL, PERL_MAGIC_uvar, get_mgvtbl(get_val, set_val), reinterpret_cast<const char*>(buffer), buffer_length);
		}
	}
}
