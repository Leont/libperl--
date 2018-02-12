#include "internal.h"
#include <perl++/perl++.h>
#include "regex_impl.h"
namespace perl {
//	const Ref<Regex>::Temp Regex::take_ref() const {
//	}

	Regex::Regex(std::unique_ptr<Implementation> _pattern) : pattern(_pattern.release()) {
	}

	const Array::Temp Regex::match(const String::Value& string, const char* flags) const {
		return implementation::Call_stack(pattern->interp).push(string, *pattern, flags).sub_list("Embed::Perlpp::match");
	}
	const Array::Temp Regex::match(const Scalar::Value& string, const char* flags) const {
		return implementation::Call_stack(pattern->interp).push(string, *pattern, flags).sub_list("Embed::Perlpp::match");
	}
	const Array::Temp Regex::match(const char* string, const char* flags) const {
		interpreter* interp = pattern->interp;
		Scalar::Temp tmp(interp, newSVpv(string, 0), true);
		return implementation::Call_stack(pattern->interp).push(tmp, *pattern, flags).sub_list("Embed::Perlpp::match");
	}
	const Array::Temp Regex::substitute(String::Value& string, const String::Value& replacement, const char* flags) const {
		return implementation::Call_stack(pattern->interp).push(string, *pattern, replacement, flags).sub_list("Embed::Perlpp::substitute");
	}
	const Array::Temp Regex::substitute(String::Value& string, Raw_string replacement, const char* flags) const {
		return implementation::Call_stack(pattern->interp).push(string, *pattern, replacement, flags).sub_list("Embed::Perlpp::substitute");
	}
	namespace implementation {
		Regex::Regex(interpreter* _interp, SV* _pattern) : interp(_interp), pattern(_pattern) {
		}
	}
}
