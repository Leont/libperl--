#include "internal.h"
#include "perl++.h"
#include "XSUB.h"
#include "regex_impl.h"

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

		void initialize_system() {
			static bool inited;
			if (!inited) {
				const char** aargs PERL_UNUSED_DECL = args;
				PERL_SYS_INIT(&arg_count, const_cast<char***>(&aargs));
				atexit(terminator);
				inited = true;
			}
		}
		interpreter* initialize_interpreter(int argc, const char* argv[]) {
			interpreter* interp = perl_alloc();
			perl_construct(interp);
			PL_exit_flags |= PERL_EXIT_DESTRUCT_END;

			perl_parse(interp, xs_init, argc, const_cast<char**>(argv), NULL);
			return interp;
		}
		interpreter* initialize_interpreter() {
			return initialize_interpreter(arg_count, args);
		}
	}

	void noop(interpreter* interp) {
	}

	void destructor(interpreter* interp) {
		perl_destruct(interp);
		perl_free(interp);
	}

	/*
	 * Class Interpreter
	 */

#define interp raw_interp.get()

	Interpreter::Interpreter() : raw_interp(initialize_interpreter(), destructor) {
		eval(implementation::to_eval);
	}
	Interpreter::Interpreter(int argc, const char* argv[]) : raw_interp(initialize_interpreter(argc, argv), destructor) {
		eval(implementation::to_eval);
	}
	Interpreter::Interpreter(interpreter* other, const override&) : raw_interp(other, noop) {
		eval(implementation::to_eval);
	}
	Interpreter Interpreter::clone() const {
		return Interpreter(perl_clone(interp, CLONEf_KEEP_PTR_TABLE), override()); // FIXME reference counting
	}
	bool operator==(const Interpreter& first, const Interpreter& second) {
		return first.raw_interp == second.raw_interp;
	}
	interpreter* Interpreter::get_interpreter() const {
		return interp;
	}
	Hash::Temp Interpreter::modglobal() const {
		return Hash::Temp(raw_interp.get(), PL_modglobal, false);
	}
	int Interpreter::run() const {
		return perl_run(raw_interp.get());
	}
	Package Interpreter::package(const char* name) const {
		return Package(interp, name);
	}
	void Interpreter::set_context() const {
		PERL_SET_CONTEXT(interp);
	}
	const Array::Temp Interpreter::list() const {
		return Array::Temp(interp, newAV(), true);
	}
	const Hash::Temp Interpreter::hash() const {
		return Hash::Temp(interp, newHV(), true);
	}

	Package Interpreter::use(const char* package_name) const {
		load_module(PERL_LOADMOD_NOIMPORT, value_of(package_name).get_SV(true), NULL, NULL);
		return package(package_name);
	}
	Package Interpreter::use(const char* package_name, double version) const {
		load_module(PERL_LOADMOD_NOIMPORT, value_of(package_name).get_SV(true), value_of(version).get_SV(true), NULL);
		return package(package_name);
	}

	const Scalar::Temp Interpreter::eval(const char* string) const {
		return eval(value_of(string));
	}

	const Scalar::Temp Interpreter::eval(const Scalar::Base& string) const {
		return implementation::Call_stack(interp).eval_scalar(string.get_SV(true));
	}

	const Array::Temp Interpreter::eval_list(const char* string) const {
		return eval_list(value_of(string));
	}
	const Array::Temp Interpreter::eval_list(const Scalar::Base& string) const {
		return implementation::Call_stack(interp).eval_list(string.get_SV(true));
	}

	Scalar::Temp Interpreter::scalar(const char* name) const {
		SV* const ret = get_sv(name, true);
		return Scalar::Temp(interp, ret, false);
	}

	Glob Interpreter::glob(const char* name) const {
		GV* const ret = gv_fetchpv(name, GV_ADD, SVt_PV);
		return Glob(raw_interp.get(), ret);
	}

	Array::Temp Interpreter::array(const char* name) const {
		AV* const ret = get_av(name, true);
		SvGETMAGIC(reinterpret_cast<SV*>(ret));
		return Array::Temp(raw_interp.get(), ret, false);
	}

	Hash::Temp Interpreter::hash(const char* name) const {
		HV* const ret = get_hv(name, true);
		SvGETMAGIC(reinterpret_cast<SV*>(ret));
		return Hash::Temp(raw_interp.get(), ret, false);
	}

	const Regex Interpreter::regex(const char* regexp) const {
		return regex(value_of(regexp));
	}

	const Regex Interpreter::regex(const String::Value& regexp) const {
#if PERL_VERSION < 9
		Scalar::Temp temp = implementation::Call_stack(raw_interp.get()).push(regexp).sub_scalar("Embed::Perlpp::regexp");
		return Regex(std::auto_ptr<Regex::Implementation>(new Regex::Implementation(raw_interp.get(), temp.release())));
#else
		return Regex(std::auto_ptr<Regex::Implementation>(new Regex::Implementation(raw_interp.get(), regexp.get_SV(true), "")));
#endif
	}

	const Scalar::Temp Interpreter::undef() const {
		return Scalar::Temp(raw_interp.get(), newSV(0), true);
	}
	const Integer::Temp Interpreter::value_of(int value) const {
		return Integer::Temp(raw_interp.get(), newSViv(value), true);
	}
	const Uinteger::Temp Interpreter::value_of(unsigned value) const {
		return Uinteger::Temp(raw_interp.get(), newSVuv(value), true);
	}
	const Number::Temp Interpreter::value_of(double value) const {
		return Number::Temp(raw_interp.get(), newSVnv(value), true);
	}
	const String::Temp Interpreter::value_of(const std::string& value) const {
		return String::Temp(raw_interp.get(), newSVpvn(value.c_str(), value.length()), true);
	}
	const String::Temp Interpreter::value_of(Raw_string value) const {
		return String::Temp(raw_interp.get(), newSVpvn(value.value, value.length), true);
	}
	const String::Temp Interpreter::value_of(const char* value) const {
		return String::Temp(raw_interp.get(), newSVpvn(value, strlen(value)), true);
	}

	Handle Interpreter::open(Raw_string filename) const {
		GV* ret = newGVgen(const_cast<char*>("Symbol"));
		bool success = do_openn(ret, const_cast<char*>(filename.value), filename.length, false, O_RDONLY, 0, Nullfp, NULL, 0);
		if (!success) {
			std::string message("Couldn't open file ");
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
		/*
		 * Class implementation::Class_temp
		 */
		Class_temp::Class_temp(interpreter* interp, const char* name) : package(interp, name, true), persistence(false), use_hash(false) {
		}
		Class_temp& Class_temp::is_persistent(bool _persistence) {
			persistence = _persistence;
			return *this;
		}
		Class_temp& Class_temp::uses_hash(bool _use_hash) {
			use_hash = _use_hash;
			return *this;
		}

		Class_state::Class_state(const char* _classname, const std::type_info& _type, MGVTBL* _magic_table, bool _is_persistent, bool _use_hash) : classname(_classname), magic_table(_magic_table), type(_type), is_persistent(_is_persistent), use_hash(_use_hash), family() {
			family.insert(&type);
		}

		MGVTBL* get_object_vtbl(const std::type_info& pre_key, int (*destruct_ptr)(interpreter*, SV*, MAGIC*)) {
			static boost::ptr_map<const std::type_info*, MGVTBL> table;
			const std::type_info* key = &pre_key;
			if (table.find(key) == table.end()) {
				const MGVTBL tmp = {0, 0, 0, 0, destruct_ptr MAGIC_TAIL};
				table.insert(key, new MGVTBL(tmp));
			}
			return &table[key];
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

	Scalar::Temp Package::scalar(const char* name) const {
		SV* const ret = get_sv((package_name + "::" + name).c_str(), true);
		return Scalar::Temp(interp, ret, false);
	}

	Array::Temp Package::array(const char* name) const {
		AV* const ret = get_av((package_name + "::" + name).c_str(), true);
		return Array::Temp(interp, ret, false);
	}

	Hash::Temp Package::hash(const char* name) const {
		HV* const ret = get_hv((package_name + "::" + name).c_str(), true);
		return Hash::Temp(interp, ret, false);
	}
}
