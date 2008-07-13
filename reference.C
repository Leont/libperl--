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
				return Perl_sv_isobject(interp, get_SV(true));
			}
			bool Reference_base::is_exactly(const char* classname) const {
				return Perl_sv_isa(interp, get_SV(true), classname);
			}
			bool Reference_base::isa(const char* classname) const {
				return Perl_sv_derived_from(interp, get_SV(true), classname);
			}
			void Reference_base::weaken() const { 
				Perl_sv_rvweaken(interp, get_SV(false));
			}
			void Reference_base::bless(const Package& type) {
				Perl_sv_bless(interp, get_SV(false), type.stash);
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
			Nonscalar<Any>::Nonscalar(interpreter* _interp, SV* _handle) : Reference_base(_interp, _handle) {
			}
			
			const std::string& Nonscalar<Any>::cast_error() {
				static const std::string message("Not a reference");
				return message;
			}

			/*
			 * Class Ref<array>::Value
			 */
			Nonscalar<Array>::Nonscalar(interpreter* _interp, SV* _handle) : Ref_specialized<Array>(_interp, _handle) {
			}
			Scalar::Temp Nonscalar<Array>::operator[](key_type index) const {
				return operator*()[index];
			}

			bool Nonscalar<Array>::is_compatible_type(const Scalar::Base& var) {
				return Nonscalar<Any>::is_compatible_type(var) and implementation::is_this_type(helper::dereference(var), SVt_PVAV);
			}

			/*
			 * Class Ref<hash>::Value
			 */
			Nonscalar<Hash>::Nonscalar(interpreter* _interp, SV* _handle) : Ref_specialized<Hash>(_interp, _handle) {
			}
			Scalar::Temp Nonscalar<Hash>::operator[](Raw_string key) const {
				return operator*()[key];
			}
			Scalar::Temp Nonscalar<Hash>::operator[](const Scalar::Base& key) const {
				return operator*()[key];
			}

			bool Nonscalar<Hash>::is_compatible_type(const Scalar::Base& var) {
				return Nonscalar<Any>::is_compatible_type(var) and is_this_type(helper::dereference(var), SVt_PVHV);
			}

			/*
			 * Class Ref<code>::Value
			 */
			Nonscalar<Code>::Nonscalar(interpreter* _interp, SV* _handle) : Ref_specialized<Code>(_interp, _handle) {
			}

			bool Nonscalar<Code>::is_compatible_type(const Scalar::Base& var) {
				return Nonscalar<Any>::is_compatible_type(var) and Code::is_storage_type(helper::dereference(var));
			}
			const std::string& Nonscalar<Code>::cast_error() {
				static const std::string message("Not an code reference");
				return message;
			}
		}
	}
}
