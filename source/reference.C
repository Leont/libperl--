#include "internal.h"
#include "perl++.h"
namespace perl {
	/*
	 * Class ref<any>
	 */

	namespace implementation {
		namespace reference {
			/* 
			 * Class Reference_base. Base of all Reference values
			 */
			Reference_base::Reference_base(interpreter* _interp, SV* _handle) : Scalar::Base(_interp, _handle) {
			}
			bool Reference_base::is_object() const {
				return sv_isobject(get_SV(true));
			}
			bool Reference_base::is_exactly(const char* classname) const {
				return sv_isa(get_SV(true), classname);
			}
			bool Reference_base::isa(const char* classname) const {
				return sv_derived_from(get_SV(true), classname);
			}
			void Reference_base::weaken() const { 
				sv_rvweaken(get_SV(false));
			}
			void Reference_base::bless(const Package& type) {
				sv_bless(get_SV(false), type.stash);
			}
			void Reference_base::bless(const char* classname) {
				bless(Package(interp, classname, false));
			}
			const char* Reference_base::get_classname() const {
				return HvNAME(SvSTASH(SvRV(get_SV(true))));
			}

			bool Reference_base::is_compatible_type(const Scalar::Base& val) {
				return SvROK(val.get_SV(false));
			}
			SV* Reference_base::copy(const Scalar::Base& val) {
				if (!is_compatible_type(val)) {
					throw Cast_exception(cast_error());
				}
				return Scalar::Base::copy_sv(val);
			}
			/*
			 * Class Ref<Any>::Value
			 */
		}
		Ref<Any>::Value::Nonscalar(interpreter* _interp, SV* _handle) : Reference_base(_interp, _handle) {
		}
		
		Ref<Any>::Value& Ref<Any>::Value::operator=(const Nonscalar<Any>& other) {
			helper::set_scalar(*this, other);
			return *this;
		}
		Ref<Any>::Value& Ref<Any>::Value::operator=(const Scalar::Temp& other) {
			if (!implementation::reference::Reference_base::is_compatible_type(other)) {
				throw Cast_exception(cast_error());
			}
			helper::set_scalar(*this, other);
			return *this;
		}
		const std::string& Ref<Any>::Value::cast_error() {
			static const std::string message("Not a reference");
			return message;
		}

		/*
		 * Class Ref<Array>::Value
		 */
		Ref<Array>::Value::Nonscalar(interpreter* _interp, SV* _handle) : Ref_specialized<Array>(_interp, _handle) {
		}
		Scalar::Temp Ref<Array>::Value::operator[](key_type index) const {
			return operator*()[index];
		}

		bool Ref<Array>::Value::is_compatible_type(const Scalar::Base& var) {
			return Nonscalar<Any>::is_compatible_type(var) and implementation::is_this_type(helper::dereference(var), SVt_PVAV);
		}

		/*
		 * Class Ref<Hash>::Value
		 */
		Ref<Hash>::Value::Nonscalar(interpreter* _interp, SV* _handle) : Ref_specialized<Hash>(_interp, _handle) {
		}
		Scalar::Temp Ref<Hash>::Value::operator[](Raw_string key) const {
			return operator*()[key];
		}
		Scalar::Temp Ref<Hash>::Value::operator[](const Scalar::Base& key) const {
			return operator*()[key];
		}

		bool Ref<Hash>::Value::is_compatible_type(const Scalar::Base& var) {
			return Ref<Any>::Value::is_compatible_type(var) and is_this_type(helper::dereference(var), SVt_PVHV);
		}

		/*
		 * Class Ref<Code>::Value
		 */
		Ref<Code>::Value::Nonscalar(interpreter* _interp, SV* _handle) : Ref_specialized<Code>(_interp, _handle) {
		}

		bool Ref<Code>::Value::is_compatible_type(const Scalar::Base& var) {
			return Ref<Any>::Value::is_compatible_type(var) and is_this_type(helper::dereference(var), SVt_PVCV);
		}
		const std::string& Ref<Code>::Value::cast_error() {
			static const std::string message("Not an code reference");
			return message;
		}

		/*
		 * Class Ref<Glob>::Value
		 */
		Ref<Glob>::Value::Nonscalar(interpreter* _interp, SV* _handle) : Ref_specialized<Glob>(_interp, _handle) {
		}

		bool Ref<Glob>::Value::is_compatible_type(const Scalar::Base& var) {
			return Ref<Any>::Value::is_compatible_type(var) and is_this_type(helper::dereference(var), SVt_PVGV);
		}
	}
}
