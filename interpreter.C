#include "internal.h"
#include "perl++.h"
#include "XSUB.h"

extern "C" {
	void boot_DynaLoader(pTHX_ CV* cv);

	static void xs_init(interpreter* interp) {
		dXSUB_SYS;
		newXS(const_cast<char*>("DynaLoader::boot_DynaLoader"), boot_DynaLoader, const_cast<char*>(__FILE__));
	}
}

namespace perl {
	namespace {
		static const char* args[] = {"", "-ew", "0"};
		static int arg_count = sizeof args / sizeof *args;

		void terminator() {
			PERL_SYS_TERM();
		}

		interpreter* initialize_interpreter() {
			static bool inited;
			if (!inited) {
				const char** aargs PERL_UNUSED_DECL = args;
				PERL_SYS_INIT(&arg_count, const_cast<char***>(&aargs));
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
		eval(implementation::to_eval);
	}
	Interpreter::Interpreter(interpreter* other) : raw_interp(other, destructor), modglobal(raw_interp.get(), PL_modglobal, false) {
		eval(implementation::to_eval);
	}
	Interpreter Interpreter::clone() const {
		return Interpreter(perl_clone(interp, CLONEf_KEEP_PTR_TABLE));
	}
	bool operator==(const Interpreter& first, const Interpreter& second) {
		return first.raw_interp == second.raw_interp;
	}
	interpreter* Interpreter::get_interpreter() const {
		return interp;
	}
	const Package Interpreter::get_package(const char* name) const {
		return Package(*this, name);
	}
	Package Interpreter::get_package(const char* name) {
		return Package(*this, name, true);
	}
	void Interpreter::set_context() const {
		PERL_SET_CONTEXT(interp);
	}
	const Array::Temp Interpreter::list() {
		return Array::Temp(interp, newAV(), true);
	}
	const Hash::Temp Interpreter::hash() {
		return Hash::Temp(interp, newHV(), true);
	}

	Package Interpreter::use(const char* package) {
		load_module(PERL_LOADMOD_NOIMPORT, value_of(package).get_SV(true), NULL, NULL);
		return get_package(package);
	}
	Package Interpreter::use(const char* package, double version) {
		load_module(PERL_LOADMOD_NOIMPORT, value_of(package).get_SV(true), value_of(version).get_SV(true), NULL);
		return get_package(package);
	}

	const Scalar::Temp Interpreter::eval(const char* string) {
		return eval(value_of(string));
	}

	const Scalar::Temp Interpreter::eval(const Scalar::Base& string) {
		return implementation::Call_stack(interp).eval_scalar(string.get_SV(true));
	}

	const Array::Temp Interpreter::eval_list(const char* string) {
		return eval_list(value_of(string));
	}
	const Array::Temp Interpreter::eval_list(const Scalar::Base& string) {
		return implementation::Call_stack(interp).eval_list(string.get_SV(true));
	}

	const Scalar::Temp Interpreter::scalar(const char* name) const {
		SV* const ret = get_sv(name, false);
		if (ret == NULL) {
			return undef();
		}
		return Scalar::Temp(interp, ret, false);
	}
	Scalar::Temp Interpreter::scalar(const char* name) {
		SV* const ret = get_sv(name, true);
		return Scalar::Temp(interp, ret, false);
	}


	const Glob Interpreter::glob(const char* name) const {
		GV* const ret = gv_fetchpv(name, GV_ADD, SVt_PV);
		return Glob(interp, ret);
	}
	Glob Interpreter::glob(const char* name) {
		GV* const ret = gv_fetchpv(name, GV_ADD, SVt_PV);
		return Glob(raw_interp.get(), ret);
	}

	const Array::Temp Interpreter::array(const char* name) const {
		AV* const ret = get_av(name, false);
		if (ret == NULL) {
			return Array::Temp(interp, newAV(), true);
		}
		SvGETMAGIC(reinterpret_cast<SV*>(ret));
		return Array::Temp(raw_interp.get(), ret, false);
	}
	Array::Temp Interpreter::array(const char* name) {
		AV* const ret = get_av(name, true);
		SvGETMAGIC(reinterpret_cast<SV*>(ret));
		return Array::Temp(raw_interp.get(), ret, false);
	}

	const Hash::Temp Interpreter::hash(const char* name) const {
		HV* const ret = get_hv(name, false);
		if (ret == NULL) {
			return Hash::Temp(raw_interp.get(), newHV(), true);
		}
		SvGETMAGIC(reinterpret_cast<SV*>(ret));
		return Hash::Temp(raw_interp.get(), ret, false);
	}
	Hash::Temp Interpreter::hash(const char* name) {
		HV* const ret = get_hv(name, true);
		SvGETMAGIC(reinterpret_cast<SV*>(ret));
		return Hash::Temp(raw_interp.get(), ret, false);
	}

	Scalar::Temp Interpreter::undef() const {
		return Scalar::Temp(raw_interp.get(), newSV(0), true);
	}
	Integer::Temp Interpreter::value_of(int value) const {
		return Integer::Temp(raw_interp.get(), newSViv(value), true);
	}
	Uinteger::Temp Interpreter::value_of(unsigned value) const {
		return Uinteger::Temp(raw_interp.get(), newSVuv(value), true);
	}
	Number::Temp Interpreter::value_of(double value) const {
		return Number::Temp(raw_interp.get(), newSVnv(value), true);
	}
	String::Temp Interpreter::value_of(const std::string& value) const {
		return String::Temp(raw_interp.get(), newSVpvn(value.c_str(), value.length()), true);
	}
	String::Temp Interpreter::value_of(Raw_string value) const {
		return String::Temp(raw_interp.get(), newSVpvn(value.value, value.length), true);
	}
	String::Temp Interpreter::value_of(const char* value) const {
		return String::Temp(raw_interp.get(), newSVpvn(value, strlen(value)), true);
	}

	Handle Interpreter::open(Raw_string filename) {
		GV* ret = newGVgen(const_cast<char*>("Symbol"));
		bool success = do_open(ret, const_cast<char*>(filename.value), filename.length, false, O_RDONLY, 0, Nullfp);
		if (!success) {
			std::string message("Couldn't open file");
			message += SvPV_nolen(ERRSV);
			throw IO_exception(message);
		}
		return Handle(interp, GvIO(ret));
	}
	Handle Interpreter::in() const {
		return Handle(interp, GvIO(PL_stdingv));
	}
	Handle Interpreter::out() const {
		return Handle(interp, GvIO(PL_defoutgv));
	}
	Handle Interpreter::err() const {
		return Handle(interp, GvIO(PL_stderrgv));
	}

#undef interp

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
			HV* const ret = gv_stashpv(name, create ? GV_ADD : 0);
			if (ret == NULL) {
				throw Runtime_exception("Package does not exist");
			}
			return ret;
		}
		HV* get_stash(interpreter* interp, SV* name, bool create) {
			HV* const ret = gv_stashsv(name, create ? GV_ADD : 0);
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
	Package::Package(interpreter* _interp, SV* _name, bool create) : interp(_interp), package_name(SvPV_nolen(_name)), stash(get_stash(interp, _name, create)) {
	}
	
	const std::string& Package::get_name() const {
		return package_name;
	}
	Package::operator const std::string&() const {
		return package_name;
	}
}
