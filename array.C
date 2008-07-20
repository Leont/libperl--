#include "internal.h"
#include "perl++.h"

//TODO: file bug report upstream
#define mg_set(a) Perl_mg_set(aTHX_ a) 

namespace perl {
	namespace {
		void copy_into(Array::Value& to, const Array::Value& from) {
			for(unsigned i = 0; i < from.length(); ++i) {
				to[i] = from[i];
			}
		}
	}

	namespace {
		AV* newav(interpreter* interp) {
			return Perl_newAV(interp);
		}
		AV* newav(interpreter* interp, const perl::Array::Value& other) {
			AV* const ret = Perl_newAV(interp);
			Array::Temp tmp(interp, ret, false);
			copy_into(tmp, other);
			return ret;
		}
	}

	namespace implementation {
		namespace array {
			/*
			 * Class Length
			 * An abstraction around array length. Supports assignment.
			 */
			Length::Length(Value& _array) : array(_array) {
			}
			Length::Length(const Length& other) : array(other.array) {
			}
			Length::operator unsigned() const {
				return const_cast<const Value&>(array).length();
			}
			Length& Length::operator=(const unsigned new_length) {
				interpreter* const interp = array.interp;
				Perl_av_fill(interp, array.handle, new_length - 1);
				return *this;
			}
		}
	}
	/*
	 * Class Array::Array::Value
	 */
	Array::Value::Value(interpreter* _interpret, AV* _handle) : interp(_interpret), handle(_handle) {
	}

	Array::Value& Array::Value::operator=(const Array::Value& other) {
		if (handle != other.handle) {
			clear();
			copy_into(*this, other);
		}
		return *this;
	}

	void Array::Value::push(const Scalar::Base& value) {
		Perl_av_push(interp, handle, Perl_newSVsv(interp, value.get_SV(true)));
	}
	void Array::Value::push(const Scalar::Temp & value) {
		SV* const tmp = value.has_ownership() ? value.release() : Perl_newSVsv(interp, value.get_SV(true));
		Perl_av_push(interp, handle, tmp);
	}
	void Array::Value::push(const Array::Value& values) {
		const unsigned size = values.length();
		for(unsigned i = 0; i < size; ++i) {
			push(values[i]);
		}
	}
	void Array::Value::push(const int val) {
		Perl_av_push(interp, handle, Perl_newSViv(interp, val));
	}
	void Array::Value::push(const unsigned val) {
		Perl_av_push(interp, handle, Perl_newSVuv(interp, val));
	}
	void Array::Value::push(const double val) {
		Perl_av_push(interp, handle, Perl_newSVnv(interp, val));
	}
	void Array::Value::push(const char* const val) {
		Perl_av_push(interp, handle, Perl_newSVpvn(interp, val, strlen(val)));
	}
	void Array::Value::push(Raw_string val) {
		Perl_av_push(interp, handle, Perl_newSVpvn(interp, val.value, val.length));
	}
	
	const Scalar::Temp Array::Value::pop() {
		return Scalar::Temp(interp, Perl_av_pop(interp, handle), true);
	}
	const Scalar::Temp Array::Value::shift() {
		return Scalar::Temp(interp, Perl_av_shift(interp, handle), true);
	}
	void Array::Value::unshift(const Scalar::Base& next_value) {
		Perl_av_unshift(interp, handle, 1);
		operator[](0) = next_value;
	}
	namespace {
		inline void unshifter(interpreter* interp, AV* handle, SV* value) {
			Perl_av_unshift(interp, handle, 1);
			Perl_av_store(interp, handle, 0, value);
			SvSETMAGIC(value);
		}
	}
	void Array::Value::unshift(const int val) {
		unshifter(interp, handle, Perl_newSViv(interp, val));
	}
	void Array::Value::unshift(const unsigned val) {
		unshifter(interp, handle, Perl_newSVuv(interp, val));
	}
	void Array::Value::unshift(const double val) {
		unshifter(interp, handle, Perl_newSVnv(interp, val));
	}
	void Array::Value::unshift(const char* const val) {
		unshifter(interp, handle, Perl_newSVpvn(interp, val, strlen(val)));
	}
	void Array::Value::unshift(Raw_string val) {
		unshifter(interp, handle, Perl_newSVpvn(interp, val.value, val.length));
	}
	void Array::Value::unshift(const Array::Value& handles) {
		const unsigned len = handles.length();
		Perl_av_unshift(interp, handle, len);
		for(unsigned i = 0; i < len; i++) {
			operator[](i) = handles[i];
		}
	}
	void Array::Value::unshift_prepare(unsigned extra_length) {
		Perl_av_unshift(interp, handle, extra_length);
	}
	
	const Scalar::Temp Array::Value::operator[](key_type index) const {
		SV* const * const ret = Perl_av_fetch(interp, handle, index, false);
		if (ret == NULL) {
			return Scalar::Temp(interp, Perl_newSV(interp, 0), true);
		}
//		SvGETMAGIC(*ret); //XXX: quite possible superfluous.
		return Scalar::Temp(interp, *ret, false);
	}

	namespace {
		int array_store(interpreter* interp, SV* var, MAGIC* magic) {
			int index = *reinterpret_cast<int*>(magic->mg_ptr);
			SV* tmp = Perl_newSVsv(interp, var);
			Perl_av_store(interp, reinterpret_cast<AV*>(magic->mg_obj), index, tmp);
			SvSETMAGIC(tmp);
			return 0;
		}
		MGVTBL array_set_magic = { 0, array_store, 0, 0, 0 };
	}

	Scalar::Temp Array::Value::operator[](const key_type index) {
		SV* const * const ret = Perl_av_fetch(interp, handle, index, false);
		if (ret == NULL) {
			SV* magical = Perl_newSV(interp, 0);
			Perl_sv_magicext(interp, magical, reinterpret_cast<SV*>(handle), PERL_MAGIC_uvar, &array_set_magic, reinterpret_cast<const char*>(&index), sizeof index);
			return Scalar::Temp(interp, magical, true, false);
		}
//		SvGETMAGIC(*ret); //XXX: quite possible superfluous.
		return Scalar::Temp(interp, *ret, false);
	}

	const Scalar::Temp Array::Value::remove(key_type index) {
		SV* ret = Perl_av_delete(interp, handle, index, 0);
		return Scalar::Temp(interp, ret, true);
	}

	bool Array::Value::exists(key_type index) const {
		return Perl_av_exists(interp, handle, index);
	}

	unsigned Array::Value::length() const {
		return Perl_av_len(interp, handle) + 1;
	}
	implementation::array::Length Array::Value::length() {
		return Length(*this);
	}

	void Array::Value::clear() {
		Perl_av_clear(interp, handle);
	}
	void Array::Value::undefine() {
		Perl_av_undef(interp, handle);
	}
	void Array::Value::extend(const unsigned new_length) {
		Perl_av_extend(interp, handle, new_length);
	}

	const String::Temp Array::Value::pack(const Raw_string pattern) const {
		if (SvMAGICAL(handle)) {
			return implementation::Call_stack(interp).push(*this).pack(pattern);
		}
		else {
			SV* ret = Perl_newSV(interp, 1);
			SV** base = AvARRAY(handle);
			Perl_packlist(interp, ret, const_cast<char*>(pattern.value), const_cast<char*>(pattern.value + pattern.length), base, base + length());
			return perl::String::Temp(interp, ret, true);
		}
	}

	void Array::Value::tie_to(const Scalar::Base& tier) {
		Perl_sv_magic(interp, reinterpret_cast<SV*>(handle), tier.get_SV(false), PERL_MAGIC_tied, "", 0);
	}
	void Array::Value::untie() {
		if (MAGIC* mg = SvRMAGICAL(reinterpret_cast<SV*>(handle)) ? Perl_mg_find(interp, reinterpret_cast<SV*>(handle), PERL_MAGIC_tied) : NULL) {
			Ref<Any> tier(Ref<Any>::Temp(interp, SvREFCNT_inc(reinterpret_cast<SV*>(mg->mg_obj)), true));
			if (tier.can("UNTIE")) {
				tier.call("UNTIE", static_cast<int>(SvREFCNT(SvRV(tier.get_SV(false)))));
			}
		}
		Perl_sv_unmagic(interp, reinterpret_cast<SV*>(handle), PERL_MAGIC_tied);
	}
	const Scalar::Temp Array::Value::tied() const {
		if (MAGIC* mg = SvRMAGICAL(reinterpret_cast<SV*>(handle)) ? Perl_mg_find(interp, reinterpret_cast<SV*>(handle), PERL_MAGIC_tied) : NULL) {
			return (mg->mg_obj != NULL) ?  Scalar::Temp(interp, SvREFCNT_inc(reinterpret_cast<SV*>(mg->mg_obj)), true) : Scalar::Temp(take_ref());
		}
		return Scalar::Temp(interp, Perl_newSV(interp, 0), true);
	}

	Array::Iterator Array::Value::begin() {
		return Iterator(*this, 0);
	}
	Array::Iterator Array::Value::end() {
		return Iterator(*this, length());
	}
	
	Array::Reverse_iterator Array::Value::rbegin() {
		return Reverse_iterator(end());
	}
	Array::Reverse_iterator Array::Value::rend() {
		return Reverse_iterator(begin());
	}

	Array::Const_iterator Array::Value::begin() const {
		return Const_iterator(*this, 0);
	}
	Array::Const_iterator Array::Value::end() const {
		return Const_iterator(*this, length());
	}

	Array::Const_reverse_iterator Array::Value::rbegin() const {
		return Const_reverse_iterator(end());
	}
	Array::Const_reverse_iterator Array::Value::rend() const {
		return Const_reverse_iterator(begin());
	}

	const std::string& Array::Value::cast_error() {
		static const std::string message("Array::Not an array");
		return message;
	}

	/*
	 * class Array::Temp
	 */
	Array::Temp::Temp(const Temp& other) : Value(other.interp, other.handle), owns(true) {
		other.owns = false;
	}
	Array::Temp::Temp(interpreter* _interp) : Value(_interp, Perl_newAV(_interp)), owns(true) {
	}
	Array::Temp::Temp(interpreter* _interp, AV* _handle, bool _owns) : Value(_interp, _handle), owns(_owns) {
	}
	void Array::Temp::release() const {
		owns = false;
	}
	Array::Temp::~Temp() {
		if (owns) {
			Perl_sv_free(interp, reinterpret_cast<SV*>(handle));
		}
	}

	/*
	 * class implementation::Array::Const_iterator
	 */
	
	Array::Const_iterator::Const_iterator(const Array::Value& _ref, Array::key_type _index) : ref(_ref), index(_index) {
	}
	Array::Const_iterator& Array::Const_iterator::operator++() {
		index++;
		return *this;
	}
	Array::Const_iterator& Array::Const_iterator::operator--() {
		index--;
		return *this;
	}
	Array::Const_iterator Array::Const_iterator::operator++(int) {
		Array::Const_iterator ret = *this;
		++*this;
		return ret;
	}
	Array::Const_iterator Array::Const_iterator::operator--(int) {
		Array::Const_iterator ret = *this;
		--*this;
		return ret;
	}
	const Scalar::Temp Array::Const_iterator::operator*() const {
		return ref[index];
	}
	Array::Const_iterator& Array::Const_iterator::operator+=(difference_type diff) {
		index += diff;
		return *this;
	}
	Array::Const_iterator& Array::Const_iterator::operator-=(difference_type diff) {
		index -= diff;
		return *this;
	}
	Array::Const_iterator Array::Const_iterator::operator+(difference_type diff) const {
		return Const_iterator(ref, index + diff);
	}
	Array::Const_iterator Array::Const_iterator::operator-(difference_type diff) const {
		return Const_iterator(ref, index - diff);
	}
	Array::Const_iterator::difference_type Array::Const_iterator::operator-(const Array::Const_iterator& other) const {
		return index - other.index;
	}
	Array::Const_iterator::value_type Array::Const_iterator::operator[](difference_type diff) const {
		return ref[index + diff];
	}

	/*
	 * class implementatoin::Array::Iterator
	 */
	
	Array::Iterator::Iterator(Array::Value& _ref, Array::key_type _index) : ref(_ref), index(_index) {
	}
	Array::Iterator& Array::Iterator::operator++() {
		index++;
		return *this;
	}
	Array::Iterator& Array::Iterator::operator--() {
		index--;
		return *this;
	}
	Array::Iterator Array::Iterator::operator++(int) {
		Array::Iterator ret = *this;
		++*this;
		return ret;
	}
	Array::Iterator Array::Iterator::operator--(int) {
		Array::Iterator ret = *this;
		--*this;
		return ret;
	}
	Scalar::Temp Array::Iterator::operator*() const {
		return ref[index];
	}
	Array::Iterator::operator Array::Const_iterator() const {
		return Const_iterator(ref, index);
	}

	Array::Iterator& Array::Iterator::operator+=(difference_type diff) {
//		assertion<Out_of_bounds_exception>(index + diff <= static_cast<signed>(ref.length()), "");
		index += diff;
		return *this;
	}
	Array::Iterator& Array::Iterator::operator-=(difference_type diff) {
		assertion<Out_of_bounds_exception>(index - diff >= -1);
		index -= diff;
		return *this;
	}
	Array::Iterator Array::Iterator::operator+(difference_type diff) const {
//		assertion<Out_of_bounds_exception>(index + diff <= static_cast<signed>(ref.length()));
		return Iterator(ref, index + diff);
	}
	Array::Iterator Array::Iterator::operator-(difference_type diff) const {
		assertion<Out_of_bounds_exception>(index - diff >= -1);
		return Iterator(ref, index - diff);
	}
	Array::Iterator::difference_type Array::Iterator::operator-(const Iterator& other) const {
		return index - other.index;
	}
	Scalar::Temp Array::Iterator::operator[](difference_type diff) const {
//		assertion<Out_of_bounds_exception>(index + diff < static_cast<signed>(ref.length()));
		return ref[index + diff];
	}

	const Ref<Array>::Temp Array::Value::take_ref() const {
		return Ref<Array>::Temp(interp, Perl_newRV(interp, reinterpret_cast<SV*>(handle)), true);
	}

    namespace implementation {
        namespace array {
            bool operator==(const Const_iterator& first, const Const_iterator& second) {
                return &first.ref == &second.ref and first.index == second.index;
            }
            bool operator!=(const Const_iterator& first, const Const_iterator& second) {
                return &first.ref != &second.ref or (&first.ref == &second.ref and first.index != second.index);
            }

            bool operator==(const Iterator& first, const Iterator& second) {
                return &first.ref == &second.ref and first.index == second.index;
            }
            bool operator!=(const Iterator& first, const Iterator& second) {
                return &first.ref != &second.ref or (&first.ref == &second.ref and first.index != second.index);
            }
        }
    }
	/* 
	 * Class array
	 */
	Array::Array(const Array& other) : Value(other.interp, newav(other.interp, other)) {
	}
	Array::Array(const Array::Temp& other) : Value(other.interp, other.owns ? other.handle : newav(other.interp, other)) {
		other.release();
	}
	Array::Array(interpreter* _interp) : Value(_interp, newav(_interp)) {
	}
	Array::~Array() {
		Perl_sv_free(interp, reinterpret_cast<SV*>(handle));
	}
	bool Array::is_storage_type(const Any::Temp& val) {
		return implementation::is_this_type(val, SVt_PVAV);
	}
}
