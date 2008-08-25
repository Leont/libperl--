#include "internal.h"
#include "perl++.h"

#ifndef do_print
#define do_print(sv, handle) Perl_do_print(aTHX_ sv, handle)
#endif
#ifndef io_close
#define io_close(io, non_implicit) Perl_io_close(aTHX_ io, non_implicit)
#endif
#define thrower assertion<IO_exception>

namespace perl {
	Handle::Handle(interpreter* _interp, IO* _handle) : interp(_interp), handle(_handle) {
	}
	Handle::Handle(const Handle& other) : interp(other.interp), handle(other.handle) {
		SvREFCNT_inc(reinterpret_cast<SV*>(handle));
	}
	PerlIO* Handle::in_handle() const {
		PerlIO* out = IoIFP(handle);
		thrower(out, "filehandle isn't readable!");
		return out;
	}
	PerlIO* Handle::out_handle() {
		PerlIO* out = IoOFP(handle);
		thrower(out, "filehandle isn't writable!");
		return out;
	}
	void Handle::print(const Scalar::Base& arg) {
		bool ret = do_print(arg.get_SV(true), out_handle());
		thrower(ret, "Could not print!");
	}
	void Handle::print(Raw_string val) {
		int ret = PerlIO_write(out_handle(), val.value, val.length);
		thrower(ret, "Could not print!");
	}
	void Handle::print(const std::string& val) {
		int ret = PerlIO_write(out_handle(), val.data(), val.length());
		thrower(ret, "Could not print!");
	}
	void Handle::print(const Array::Value& array) {
		for (unsigned i = 0; i < array.length(); i++) {
			print(array[i]);
		}
	}
	void Handle::close() {
		bool ret = io_close(handle, true);
		thrower(ret, "Could not close!");
		IoTYPE(handle) = IoTYPE_CLOSED;
	}
	Handle::~Handle() {
		if (is_open()) {
			close();
		}
	}
	String::Temp Handle::read(unsigned length) {
		if (length == 0) {
			return String::Temp(interp, newSVpvn("", 0), true);
		}
		SV* ret = newSV(length);
		int count = PerlIO_read(in_handle(), SvPVX(ret), length);
		SvPVX(ret)[SvCUR(ret)] = '\0';
		SvCUR_set(ret, count);
		return String::Temp(interp, ret, true);
	}
	bool Handle::eof() const {
		return PerlIO_eof(in_handle());
	}
	bool Handle::is_open() const {
		return is_readable() || is_writable();
	}
	bool Handle::is_writable() const {
		return IoOFP(handle) != NULL;
	}
	bool Handle::is_readable() const {
		return IoIFP(handle) != NULL;
	}
}
