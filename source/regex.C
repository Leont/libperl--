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
	
	int Regex::match(const String::Value& string) const {
		return implementation::Call_stack(pattern->interp).match_scalar(pattern->pattern, string, 0);
	}
	int Regex::substitute(String::Value& string, const String::Value& replacement) const {
		return 0;
	}
	int Regex::substitute(String::Value& string, Raw_string replacement) const {
		return 0;
	}
	namespace {
		UV get_flags(const char*) {
			return 0;
		}
		REGEXP* regcomp(interpreter* interp, SV* pattern, const char* _flags) {
			UV flags = get_flags(_flags);
			return CALLREGCOMP(pattern, flags);
		}
	}
	namespace implementation {
		Regex::Regex(interpreter* _interp, SV* _pattern, const char* flags) : interp(_interp), pattern(regcomp(_interp, _pattern, flags)) {
		}
	}
#endif
}
