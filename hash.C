#include "internal.h"
#include "perl++.h"
#define sv_2pv_flags(a,b,c) Perl_sv_2pv_flags(aTHX_ a,b,c)
#define sv_2mortal(a) Perl_sv_2mortal(aTHX_ a)
#define newSVpvn(a,b) Perl_newSVpvn(aTHX_ a,b)
#define mg_set(a) Perl_mg_set(aTHX_ a) 

namespace perl {
	namespace {
		HV* copy_to(HV* to, interpreter* interp, HV* from) {
			Perl_hv_iterinit(interp, from);
			while (HE* iterator = Perl_hv_iternext(interp, from)) {
				if (HeKLEN(iterator) == HEf_SVKEY) {
					Perl_hv_store_ent(interp, to, SvREFCNT_inc(HeSVKEY(iterator)), SvREFCNT_inc(HeVAL(iterator)), HeHASH(iterator));
				} 
				else {
					STRLEN len;
					char* tmp = HePV(iterator, len);
					Perl_hv_store(interp, to, tmp, len, SvREFCNT_inc(HeVAL(iterator)), HeHASH(iterator));
				}
			}
			return to;
		}
	}
	namespace implementation {
		namespace hash {
			/*
			 * Class Hash::Value
			 */
			Value::Value(interpreter* _interp, HV* _handle) : interp(_interp), handle(_handle) {
			}
			Value& Value::operator=(const Value& other) {
				if (handle != other.handle) {
					clear();
					copy_to(handle, interp, other.handle);
				}
				return *this;
			}

			const Scalar::Temp Value::operator[](const Raw_string index) const {
				SV* const * const ret = Perl_hv_fetch(interp, handle, index.value, index.length, false);
				if (!ret) {
					return Scalar::Temp(interp, Perl_newSV(interp, 0), true);
				}
				SvGETMAGIC(*ret);
				return Scalar::Temp(interp, *ret, false);
			}

			namespace {
				int string_store(interpreter* interp, SV* var, MAGIC* magic) {
					SV* tmp = Perl_newSVsv(interp, var);
					Perl_hv_store(interp, reinterpret_cast<HV*>(magic->mg_obj), magic->mg_ptr, magic->mg_len, tmp, 0);
					SvSETMAGIC(tmp);
					return 0;
				}
				MGVTBL string_magic = { 0, string_store, 0, 0, 0 };
			}
			Scalar::Temp Value::operator[](const Raw_string index) {
				SV* const * const ret = Perl_hv_fetch(interp, handle, index.value, index.length, false);
				if (!ret) {
					SV* magical = Perl_newSV(interp, 0);
					Perl_sv_magicext(interp, magical, reinterpret_cast<SV*>(handle), PERL_MAGIC_uvar, &string_magic, index.value, index.length);
					return Scalar::Temp(interp, magical, true, false);
				}
				SvGETMAGIC(*ret);
				return Scalar::Temp(interp, *ret, false);
			}


			const Scalar::Temp Value::operator[](const Scalar::Base& key) const {
				HE* const entry = Perl_hv_fetch_ent(interp, handle, key.get_SV(true), false, 0);
				if (!entry) {
					return Scalar::Temp(interp, Perl_newSV(interp, 0), true);
				}
				SV* const ret = HeVAL(entry);
				SvGETMAGIC(ret);
				return Scalar::Temp(interp, ret, false);
			}
			namespace {
				int scalar_store(interpreter* interp, SV* var, MAGIC* magic) {
					SV* tmp = Perl_newSVsv(interp, var);
					Perl_hv_store_ent(interp, reinterpret_cast<HV*>(magic->mg_obj), *reinterpret_cast<SV**>(magic->mg_ptr), tmp, 0);
					SvSETMAGIC(tmp);
					return 0;
				}
				MGVTBL scalar_magic = { 0, string_store, 0, 0, 0 };
			}
			Scalar::Temp Value::operator[](const Scalar::Base& key) {
				HE* const entry = Perl_hv_fetch_ent(interp, handle, key.get_SV(true), false, 0);
				if (!entry) {
					SV* magical = Perl_newSV(interp, 0);
					Perl_sv_magicext(interp, magical, reinterpret_cast<SV*>(handle), PERL_MAGIC_uvar, &scalar_magic, reinterpret_cast<const char*>(key.get_SV(false)), sizeof(SV));
					return Scalar::Temp(interp, magical, true, false);
				}
				SV* const ret = HeVAL(entry);
				SvGETMAGIC(ret);
				return Scalar::Temp(interp, ret, false);
			}
			
			const Array::Temp Value::keys() const {
				Array::Temp ret(interp);
				foreach_init();
				while (const Iterator pair = next_value()) {
					ret.push(pair.key().as_scalar());
				}
				return ret;
			}
			const Array::Temp Value::values() const {
				Array::Temp ret(interp);
				foreach_init();
				while (const Iterator pair = next_value()) {
					ret.push(pair.value());
				}
				return ret;
			}

			bool Value::exists(Raw_string index) {
				return Perl_hv_exists(interp, handle, index.value, index.length);
			}

			bool Value::exists(const Scalar::Base& index) {
				return Perl_hv_exists_ent(interp, handle, index.get_SV(true), 0);
			}

			const Scalar::Temp Value::erase(Raw_string index) {
				SV* const tmp = Perl_hv_delete(interp, handle, index.value, index.length, 0);
				return Scalar::Temp(interp, tmp, true);
			}
			const Scalar::Temp Value::erase(const Scalar::Base& index) {
				SV* const tmp = Perl_hv_delete_ent(interp, handle, index.get_SV(true), 0, 0);
				return Scalar::Temp(interp, tmp, true);
			}
			void Value::clear() {
				Perl_hv_clear(interp, handle);
			}

			void Value::undefine() {
				Perl_hv_undef(interp, handle);
			}

			void Value::foreach_init() const {
				Perl_hv_iterinit(interp, handle);
			}

			const Iterator Value::next_value() const {
				return Iterator(interp, Perl_hv_iternext(interp, handle));
			}
			Iterator Value::next_value() {
				return Iterator(interp, Perl_hv_iternext(interp, handle));
			}

			const std::string& Value::cast_error() {
				static const std::string message("Not a hash");
				return message;
			}

			/*
			 * Class Hash::Iterator
			 */

			Iterator::Iterator(interpreter* _interp, HE* iter) : interp(_interp), iterator(iter) {
			}
			const Iterator::Key_type Iterator::key() const {
				return Key_type(*this);
			}
			const Scalar::Temp Iterator::value() const {
				SV* const ret = HeVAL(iterator);
				SvGETMAGIC(ret);
				return Scalar::Temp(interp, ret, false);
			}
			Scalar::Temp Iterator::value() {
				return Scalar::Temp(interp, HeVAL(iterator), false);
			}
			Iterator::operator bool() const {
				return iterator != NULL;
			}
			uint32_t Iterator::hashcode() const {
				return HeHASH(iterator);
			}

			/*
			 * Class Hash::Iterator::Key_type
			 */
			Iterator::Key_type::Key_type(const Iterator& _ref) : ref(_ref) {
			}

			Iterator::Key_type::operator Raw_string() const {
				return as_raw_string();
			}
			Iterator::Key_type::operator const char*() const {
				return as_raw_string();
			}
			Iterator::Key_type::operator const Scalar::Temp() const {
				return as_scalar();
			}

			#define interp ref.interp
			Raw_string Iterator::Key_type::as_raw_string() const {
				STRLEN length;
				const char* const tmp = HePV(ref.iterator, length);
				return Raw_string(tmp, length, true); // Is this unicode?
			}
			const Scalar::Temp Iterator::Key_type::as_scalar() const {
				return Scalar::Temp(interp, HeSVKEY_force(ref.iterator), false);
			}
			#undef interp

			std::ostream& operator<<(std::ostream& stream, const Iterator::Key_type& value) {
				return stream << value.as_raw_string();
			}
		}
	}

	const Ref<Hash>::Temp take_ref(const Hash::Value& var) {
		return Ref<Hash>::Temp(var.interp, Perl_newRV(var.interp, reinterpret_cast<SV*>(var.handle)), true);
	} 

	/*
	 * Class Hash::Temp
	 */
	Hash::Temp::Temp(const Temp& other) : Value(other.interp, other.handle), owns(true) {
		other.owns = false;
	}
	Hash::Temp::Temp(interpreter* _interp, HV* _handle, bool _owns) : Value(_interp, _handle), owns(_owns) {
	}
	void Hash::Temp::release() const {
		owns = false;
	}
	Hash::Temp::~Temp() {
		if (owns) {
			Perl_sv_free(interp, reinterpret_cast<SV*>(handle));
		}
	}

	Hash::Hash(const Interpreter& _interp) : Value(_interp.get_interpreter(), Perl_newHV(_interp.get_interpreter())) {
	}
	Hash::Hash(const Hash& other) : Value(other.interp, Perl_newHVhv(other.interp, other.handle)) {
	}
	Hash::Hash(const Temp& other) : Value(other.interp, other.owns ? other.handle : Perl_newHVhv(other.interp, other.handle)) {
		other.release();
	}

	Hash::~Hash() {
		Perl_sv_free(interp, reinterpret_cast<SV*>(handle));
	}

	bool Hash::is_storage_type(const Any::Temp& var) {
		return implementation::is_this_type(var, SVt_PVHV);
	}
} // namespace

