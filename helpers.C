#include "internal.h"
#include "perl++.h"

namespace perl {
	Raw_string::Raw_string(const char* _value) : value(_value), length(std::strlen(value)), utf8(false) {
	}
	Raw_string::Raw_string(const char* _value, unsigned _length, bool _utf8) : value(_value), length(_length), utf8(_utf8) {
	}
	Raw_string::Raw_string(const std::string& val) : value(val.c_str()), length(val.length()), utf8(false) {
	}
	Raw_string::operator const char*() const {
		return value;
	}
	std::string Raw_string::to_string() const {
		return std::string(value, length);
	}

	bool operator==(const Raw_string left, const Raw_string right) {
		if (left.length != right.length) {
			return false;
		}
		return strnEQ(left.value, right.value, MIN(left.length, right.length) + 1);
	}
	bool operator==(const Raw_string left, const char* right) {
		return strnEQ(left.value, right, left.length + 1);
	}
	bool operator==(const char* left, const Raw_string right) {
		return strnEQ(left, right.value, right.length + 1);
	}
	bool operator!=(const Raw_string left, const Raw_string right) {
		if (left.length != right.length) {
			return true;
		}
		return strnNE(left, right, MIN(left.length, right.length));
	}
	bool operator<(const Raw_string left, const Raw_string right) {
		return strLE(left, right);
	}
	bool operator>(const Raw_string left, const Raw_string right) {
		return strGT(left, right);
	}
	bool operator<=(const Raw_string left, const Raw_string right) {
		return strLE(left, right);
	}
	bool operator>=(const Raw_string left, const Raw_string right) {
		return strGE(left, right);
	}

	std::ostream& operator<<(std::ostream& stream, const Raw_string data) {
		return stream.write(data.value, data.length);
	}
}
