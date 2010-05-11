#include "internal.h"
#include "perl++.h"
#include "regex_impl.h"
namespace perl {
//	const Ref<Regex>::Temp Regex::take_ref() const {
//	}

	Regex::Regex(std::auto_ptr<Implementation> _pattern) : pattern(_pattern.release()) {
	}

	int Regex::match(const String::Value& string, const char* flags) const {
		return implementation::Call_stack(pattern->interp).push(string, *pattern, flags).sub_scalar("Embed::Perlpp::match");
	}
	int Regex::match(const char* string, const char* flags) const {
		interpreter* interp = pattern->interp;
		Scalar::Temp tmp(interp, newSVpv(string, 0), true);
		return implementation::Call_stack(pattern->interp).push(tmp, *pattern, flags).sub_scalar("Embed::Perlpp::match");
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

	/*
	int Regex::match(const String::Value& string, const char* flags) const {
		return implementation::Call_stack(pattern->interp).match_scalar(pattern->pattern, string, flags);
	}
	int Regex::match(const Scalar::Value& string, const char* flags) const {
		return implementation::Call_stack(pattern->interp).match_scalar(pattern->pattern, string, flags);
	}
	int Regex::match(const char* string, const char* flags) const {
		interpreter* interp = pattern->interp;
		return implementation::Call_stack(pattern->interp).match_scalar(pattern->pattern, String::Temp(interp, newSVpv(string, 0), true), flags);
	}
	*/

	/* XXX
	const Array::Temp Regex::comb(const String::Value& string, const char* flags) const {
		return implementation::Call_stack(pattern->interp).match_list(pattern->pattern, string, flags);
	}
	const Array::Temp Regex::comb(const Scalar::Value& string, const char* flags) const {
		return implementation::Call_stack(pattern->interp).match_list(pattern->pattern, string, flags);
	}
	const Array::Temp Regex::comb(const char* string, const char* flags) const {
		interpreter* interp = pattern->interp;
		return implementation::Call_stack(pattern->interp).match_list(pattern->pattern, String::Temp(interp, newSVpv(string, 0), true), flags);
	}
	*/
}
