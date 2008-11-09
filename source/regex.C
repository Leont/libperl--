#include "internal.h"
#include "perl++.h"
#include "regex_impl.h"
namespace perl {
//	const Ref<Regex>::Temp Regex::take_ref() const {
//	}
	Regex::Regex(std::auto_ptr<Implementation> _pattern) : pattern(_pattern.release()) {
	}
	bool Regex::match(const String::Value& string) const {
		return implementation::Call_stack(pattern->interp).push(string, *pattern).sub_scalar("Embed::Perlpp::match");
	}
	bool Regex::substitute(String::Value& string, const String::Value& replacement) const {
		return implementation::Call_stack(pattern->interp).push(string, *pattern, replacement).sub_scalar("Embed::Perlpp::substitute");
	}
	bool Regex::substitute(String::Value& string, Raw_string replacement) const {
		return implementation::Call_stack(pattern->interp).push(string, *pattern, replacement).sub_scalar("Embed::Perlpp::substitute");
	}
	namespace implementation {
		Regex::Regex(interpreter* _interp, SV* _pattern) : interp(_interp), pattern(_pattern) {
		}
	}
}
