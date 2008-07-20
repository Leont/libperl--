#include "internal.h"
#include "perl++.h"

#define save_int(a)     Perl_save_int(raw_interp.get(), a)
#define free_tmps()     Perl_free_tmps(raw_interp.get())
#define mg_set(a)       Perl_mg_set(aTHX_ a)
#define sv_2bool(a)     Perl_sv_2bool(aTHX_ a)
#define sv_setsv_flags(a,b,c)   Perl_sv_setsv_flags(aTHX_ a,b,c)
#define sv_2pv_flags(a,b,c) Perl_sv_2pv_flags(aTHX_ a,b,c)

extern "C" {
	void boot_DynaLoader(pTHX_ CV* cv);

	static void xs_init(pTHX) {
		dXSUB_SYS;
		Perl_newXS(my_perl, const_cast<char*>("DynaLoader::boot_DynaLoader"), boot_DynaLoader, const_cast<char*>(__FILE__));
	}
}

namespace perl {
	namespace {
		static const char* args[] = {"", "-e", "0"};
		static int arg_count = sizeof args / sizeof *args;

		void terminator() {
			PERL_SYS_TERM();
		}

		interpreter* initialize_interpreter() {
			static bool inited;
			if (!inited) {
				
				PERL_SYS_INIT(&arg_count, &const_cast<char**>(args));
				atexit(terminator);
//#ifdef __GNUC__
//				std::set_terminate(__gnu_cxx::__verbose_terminate_handler);
//#endif
				inited = true;
			}
			interpreter* interp = perl_alloc();
			perl_construct(interp);
			PL_exit_flags |= PERL_EXIT_DESTRUCT_END;

			perl_parse(interp, xs_init, arg_count, const_cast<char**>(args), NULL);
			return interp;
		}
	}

	void destructor(interpreter* interp) {
		perl_destruct(interp);
		perl_free(interp);
	}

	/*
	 * Class Interpreter
	 */

#define interp raw_interp.get()

	Interpreter::Interpreter() : raw_interp(initialize_interpreter(), destructor), modglobal(raw_interp.get(), PL_modglobal, false) {
	}
	Interpreter::Interpreter(interpreter* other) : raw_interp(other, destructor), modglobal(raw_interp.get(), PL_modglobal, false) {
	}
	Interpreter Interpreter::clone() const {
		return Interpreter(perl_clone(raw_interp.get(), CLONEf_KEEP_PTR_TABLE));
	}
	bool operator==(const Interpreter& first, const Interpreter& second) {
		return first.raw_interp == second.raw_interp;
	}
	interpreter* Interpreter::get_interpreter() const {
		return raw_interp.get();
	}
	const Package Interpreter::get_package(const char* name) const {
		return Package(*this, name);
	}
	Package Interpreter::get_package(const char* name) {
		return Package(*this, name, true);
	}
	void Interpreter::set_context() const {
		PERL_SET_CONTEXT(raw_interp.get());
	}
	const Array::Temp Interpreter::list() {
		return Array::Temp(raw_interp.get(), Perl_newAV(raw_interp.get()), true);
	}
	const Hash::Temp Interpreter::hash() {
		return Hash::Temp(raw_interp.get(), Perl_newHV(raw_interp.get()), true);
	}

	Package Interpreter::use(const char* package) {
		Perl_load_module(raw_interp.get(), 0, value_of(package).get_SV(true), NULL, NULL);
		return get_package(package);
	}
	Package Interpreter::use(const char* package, double version) {
		Perl_load_module(raw_interp.get(), 0, value_of(package).get_SV(true), value_of(version).get_SV(true), NULL);
		return get_package(package);
	}

	const Scalar::Temp Interpreter::eval(const char* string) {
		SAVETMPS;
		SV* const ret = SvREFCNT_inc(Perl_eval_pv(raw_interp.get(), string, FALSE));
		FREETMPS;
		if (SvTRUE(ERRSV)) {
			STRLEN length;
			const char* message = SvPV(ERRSV, length);
			throw Runtime_exception(message, length);
		}
		return Scalar::Temp(raw_interp.get(), ret, true);
	}

	const Scalar::Temp Interpreter::eval(const Scalar::Base& sv) {
		SAVETMPS;
		SV* const ret = SvREFCNT_inc(Perl_eval_sv(raw_interp.get(), sv.get_SV(true), G_SCALAR));
		FREETMPS;
		if (SvTRUE(ERRSV)) {
			STRLEN length;
			const char* message = SvPV(ERRSV, length);
			throw Runtime_exception(message, length);
		}
		return Scalar::Temp(raw_interp.get(), ret, true);
	}

	const Scalar::Temp Interpreter::scalar(const char* name) const {
		SV* const ret = Perl_get_sv(raw_interp.get(), name, FALSE);
		if (ret == NULL) {
			return undef();
		}
//		SvGETMAGIC(ret);
		return Scalar::Temp(raw_interp.get(), ret, false);
	}

#undef interp

	namespace {
		int scalar_store(interpreter* interp, SV* var, MAGIC* magic) {
			SvSetMagicSV(Perl_get_sv(interp, magic->mg_ptr, true), var);
			return 0;
		}
		MGVTBL scalar_set_magic = { 0, scalar_store, 0, 0, 0 };
	}
	Scalar::Temp Interpreter::scalar(const char* name) {
		SV* const ret = Perl_get_sv(raw_interp.get(), name, false);
		if (ret == NULL) {
			SV* magical = Perl_newSV(raw_interp.get(), 0);
			Perl_sv_magicext(raw_interp.get(), magical, NULL, PERL_MAGIC_uvar, &scalar_set_magic, name, strlen(name) + 1);
			return Scalar::Temp(raw_interp.get(), magical, true, false);
		}
		return Scalar::Temp(raw_interp.get(), ret, false);
	}

	static inline void get_magic(interpreter* interp, SV* handle) {
		if (SvGMAGICAL(handle)) Perl_mg_get(interp, handle);
	}

	const Glob Interpreter::glob(const char* name) const {
		GV* const ret = Perl_gv_fetchpv(raw_interp.get(), name, GV_ADD, SVt_PV);
		return Glob(raw_interp.get(), ret);
	}
	Glob Interpreter::glob(const char* name) {
		GV* const ret = Perl_gv_fetchpv(raw_interp.get(), name, GV_ADD, SVt_PV);
		return Glob(raw_interp.get(), ret);
	}
	const Array::Temp Interpreter::array(const char* name) const {
		AV* const ret = Perl_get_av(raw_interp.get(), name, false);
		if (ret == NULL) {
			return Array::Temp(raw_interp.get(), Perl_newAV(raw_interp.get()), true);
		}
		get_magic(raw_interp.get(), reinterpret_cast<SV*>(ret));
		return Array::Temp(raw_interp.get(), ret, false);
	}

	Hash::Value Interpreter::hash(const char* name) const {
		HV* const ret = Perl_get_hv(raw_interp.get(), name, false);
		if (ret == NULL) {
			return Hash::Temp(raw_interp.get(), Perl_newHV(raw_interp.get()), true);
		}
		get_magic(raw_interp.get(), reinterpret_cast<SV*>(ret));
		return Hash::Temp(raw_interp.get(), ret, false);
	}

	Scalar::Temp Interpreter::undef() const {
		return Scalar::Temp(raw_interp.get(), Perl_newSV(raw_interp.get(), 0), true);
	}
	Integer::Temp Interpreter::value_of(int value) const {
		return Integer::Temp(raw_interp.get(), Perl_newSViv(raw_interp.get(), value), true);
	}
	Uinteger::Temp Interpreter::value_of(unsigned value) const {
		return Uinteger::Temp(raw_interp.get(), Perl_newSVuv(raw_interp.get(), value), true);
	}
	Number::Temp Interpreter::value_of(double value) const {
		return Number::Temp(raw_interp.get(), Perl_newSVnv(raw_interp.get(), value), true);
	}
	String::Temp Interpreter::value_of(const std::string& value) const {
		return String::Temp(raw_interp.get(), Perl_newSVpvn(raw_interp.get(), value.c_str(), value.length()), true);
	}
	String::Temp Interpreter::value_of(Raw_string value) const {
		return String::Temp(raw_interp.get(), Perl_newSVpvn(raw_interp.get(), value.value, value.length), true);
	}
	String::Temp Interpreter::value_of(const char* value) const {
		return String::Temp(raw_interp.get(), Perl_newSVpvn(raw_interp.get(), value, strlen(value)), true);
	}

	namespace implementation {
		namespace classes {
			/*
			 * Class implementation::classes::Temp
			 */
			Temp::Temp(const Interpreter& interp, const char* name) : package(interp, name, true) {
			}

			Type_info::Type_info(const std::type_info& _raw) : raw(_raw) {
			}
			bool Type_info::operator<(const Type_info& other) const {
				return &raw < &other.raw;
			}
			Table::~Table() {
				typedef std::map<Type_info, MGVTBL*>::iterator iterator;
				for(iterator current = table.begin(); current != table.end(); ++current) {
					delete current->second;
				}
			}
			void Table::add(const Type_info& type, int (*destruct_ptr)(interpreter*, SV*, MAGIC*)) {
				MGVTBL* tmp = new MGVTBL();
				tmp->svt_free = destruct_ptr;
				table[type] = tmp;
			}
		}
	}
	/*
	 * Class Package
	 */
	namespace {
		HV* get_stash(interpreter* interp, const char* name, bool create) {
			HV* const ret = Perl_gv_stashpv(interp, name, create);
			if (ret == NULL) {
				throw Runtime_exception("Package does not exist");
			}
			return ret;
		}
	}
	Package::Package(const Package& other) : interp(other.interp), package_name(other.package_name), stash(other.stash) {
	}
	Package::Package(const Interpreter& _interp, const char* _name, bool create) : interp(_interp.get_interpreter()), package_name(_name), stash(get_stash(interp, _name, create)) {
	}
	Package::Package(interpreter* _interp, const char* _name, bool create) : interp(_interp), package_name(_name), stash(get_stash(interp, _name, create)) {
	}
	
	const std::string& Package::get_name() const {
		return package_name;
	}
	Package::operator const std::string&() const {
		return package_name;
	}
}
