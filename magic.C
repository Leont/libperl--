#include "internal.h"
#include "perl++.h"

namespace perl {
	namespace implementation {
		void* get_magic_ptr(const MAGIC* magic) {
			return magic->mg_ptr;
		}
		void* get_magic_ptr(interpreter* interp, SV* var, int min_length) {
			MAGIC* tmp = Perl_mg_find(interp, var, PERL_MAGIC_ext);
			if (tmp == NULL || tmp->mg_ptr == NULL || tmp->mg_len < min_length) {
				throw Runtime_exception("Magic error");
			}
			return tmp->mg_ptr;
		}
		const Raw_string get_magic_string(interpreter* interp, SV* var) {
			MAGIC* tmp = Perl_mg_find(interp, var, PERL_MAGIC_ext);
			return tmp != NULL ? Raw_string(tmp->mg_ptr, tmp->mg_len, false) : Raw_string(NULL, 0, false);
		}
		bool has_magic_string(interpreter* interp, SV* var) {
			return Perl_mg_find(interp, var, PERL_MAGIC_ext) != NULL;
		}
		bool has_magic_string(const Scalar::Base& var) {
			return Perl_mg_find(var.interp, var.get_SV(false), PERL_MAGIC_ext) != NULL;
		}
		void set_magic_string(interpreter* interp, SV* var, Raw_string string) {
			Perl_sv_magic(interp, var, NULL, PERL_MAGIC_ext, string.value, string.length);
		}
		void set_magic_string(interpreter* interp, SV* var, const void* value, unsigned length) {
			Perl_sv_magic(interp, var, NULL, PERL_MAGIC_ext, reinterpret_cast<const char*>(value), length);
		}

		SV* make_magic_object(interpreter* interp, const void* obj, const classes::State& state) {
			SV* const referee = state.hash ? reinterpret_cast<SV*>(Perl_newHV(interp)) : Perl_newSV(interp, 0);
			Object_buffer buffer(obj, true);
			Perl_sv_magicext(interp, referee, NULL, PERL_MAGIC_ext, state.magic_table, reinterpret_cast<const char*>(&buffer), sizeof buffer);
			SV* const ret = Perl_newRV_noinc(interp, referee);
			Perl_sv_bless(interp, ret, Perl_gv_stashpv(interp, state.classname, true));
			return ret;
		}
		void** get_magic_object_impl(interpreter* interp, SV* var, int min_length) {
			MAGIC* tmp = Perl_mg_find(interp, SvRV(var), PERL_MAGIC_ext);
			if (tmp == NULL || tmp->mg_ptr == NULL || tmp->mg_len < min_length) {
				throw Not_an_object_exception();
			}
			return reinterpret_cast<void**>(tmp->mg_ptr);
		}

		class Getset_table {
			typedef std::pair<magic_fun, magic_fun> func_pair;
			std::map<func_pair, MGVTBL*> magic_cache;
			MGVTBL* make_magic_table(const func_pair& key) {
				MGVTBL* ret = new MGVTBL();
				memset(ret, 0, sizeof ret);
				ret->svt_get = key.first;
				ret->svt_set = key.second;
				return ret;
			}
			public:
			MGVTBL* get(magic_fun get_val, magic_fun set_val) {
				func_pair key(get_val, set_val);
				if (magic_cache.find(key) == magic_cache.end()) {
					magic_cache[key] = make_magic_table(key);
				}
				return magic_cache[key];
			}
			~Getset_table() {
				typedef std::map<func_pair, MGVTBL*>::iterator iterator;
				for(iterator current = magic_cache.begin(); current != magic_cache.end(); ++current) {
					delete current->second;
				}
			}
		};
		static Getset_table table;

		void attach_getset_magic(interpreter* interp, SV* var, magic_fun get_val, magic_fun set_val, const void* buffer, size_t buffer_length) {
			Perl_sv_magicext(interp, var, NULL, PERL_MAGIC_uvar, table.get(get_val, set_val), reinterpret_cast<const char*>(buffer), buffer_length);
		}
	}
}
