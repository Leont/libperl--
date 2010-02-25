#include "internal.h"
#include "perl++.h"
#include "regex_impl.h"
namespace perl {
//	const Ref<Regex>::Temp Regex::take_ref() const {
//	}
	Regex::Regex(std::auto_ptr<Implementation> _pattern) : pattern(_pattern.release()) {
	}
#if PERL_VERSION < 9
	int Regex::match(const String::Value& string) const {
		return implementation::Call_stack(pattern->interp).push(string, *pattern).sub_scalar("Embed::Perlpp::match");
	}
	int Regex::substitute(String::Value& string, const String::Value& replacement) const {
		return implementation::Call_stack(pattern->interp).push(string, *pattern, replacement).sub_scalar("Embed::Perlpp::substitute");
	}
	int Regex::substitute(String::Value& string, Raw_string replacement) const {
		return implementation::Call_stack(pattern->interp).push(string, *pattern, replacement).sub_scalar("Embed::Perlpp::substitute");
	}
	namespace implementation {
		Regex::Regex(interpreter* _interp, SV* _pattern) : interp(_interp), pattern(_pattern) {
		}
	}
#else 
	namespace {
		UV get_flags(const char* flag_string) {
			int flags;
			for (char current_flag = *flag_string; *flag_string; flag_string++) {
				switch(current_flag) {
					case IGNORE_PAT_MOD:    flags |= RXf_PMf_FOLD;       break;
					case MULTILINE_PAT_MOD: flags |= RXf_PMf_MULTILINE;  break;
					case SINGLE_PAT_MOD:    flags |= RXf_PMf_SINGLELINE; break;
					case XTENDED_PAT_MOD:   flags |= RXf_PMf_EXTENDED;   break;
					case GLOBAL_PAT_MOD:    flags |= PMf_GLOBAL;         break;
					case CONTINUE_PAT_MOD:  flags |= PMf_CONTINUE;       break;
					case ONCE_PAT_MOD:      flags |= PMf_KEEP;           break;
					case KEEPCOPY_PAT_MOD:  flags |= PMf_KEEPCOPY;       break;
				}
			}
			return flags;
		}
		REGEXP* regcomp(interpreter* interp, SV* pattern, const char* flag_string) {
			UV flags = get_flags(flag_string);
			return CALLREGCOMP(pattern, flags);
		}
	}

	int Regex::match(const String::Value& string, const char* flags) const {
		return implementation::Call_stack(pattern->interp).match_scalar(pattern->pattern, string, get_flags(flags));
	}
	int Regex::match(const Scalar::Value& string, const char* flags) const {
		return implementation::Call_stack(pattern->interp).match_scalar(pattern->pattern, string, get_flags(flags));
	}
	int Regex::match(const char* string, const char* flags) const {
		interpreter* interp = pattern->interp;
		return implementation::Call_stack(pattern->interp).match_scalar(pattern->pattern, String::Temp(interp, newSVpv(string, 0), true), get_flags(flags));
	}

	const Array::Temp Regex::comb(const String::Value& string, const char* flags) const {
		return implementation::Call_stack(pattern->interp).match_list(pattern->pattern, string, get_flags(flags));
	}
	const Array::Temp Regex::comb(const Scalar::Value& string, const char* flags) const {
		return implementation::Call_stack(pattern->interp).match_list(pattern->pattern, string, get_flags(flags));
	}
	const Array::Temp Regex::comb(const char* string, const char* flags) const {
		interpreter* interp = pattern->interp;
		return implementation::Call_stack(pattern->interp).match_list(pattern->pattern, String::Temp(interp, newSVpv(string, 0), true), get_flags(flags));
	}

	int Regex::substitute(String::Value& string, const String::Value& replacement) const {
		return implementation::Call_stack(pattern->interp).subst_scalar(pattern->pattern, string, replacement, 0);
	}
	int Regex::substitute(String::Value& string, Raw_string replacement) const {
		interpreter* interp = pattern->interp;
		SV* replacement_sv = newSVpvn(replacement.value, replacement.length);
		return implementation::Call_stack(interp).subst_scalar(pattern->pattern, string, String::Temp(interp, replacement_sv, true), 0);
	}
	namespace implementation {
		Regex::Regex(interpreter* _interp, SV* _pattern, const char* flags) : interp(_interp), pattern(regcomp(_interp, _pattern, flags)) {
		}
	}
#endif
}
