#include "internal.h"
#include "perl++.h"

namespace perl {
	namespace {
		void thrower(bool ret, const char* message) {
			if (!ret) {
				throw IO_exception(message);
			}
		}
	}
	Handle::Handle(interpreter* _interp, IO* _handle) : interp(_interp), handle(_handle) {
	}
	Handle::Handle(const Handle& other) : interp(other.interp), handle(other.handle) {
		SvREFCNT_inc(reinterpret_cast<SV*>(handle));
	}
	PerlIO* Handle::in_handle() const {
		PerlIO* out = IoOFP(handle);
		thrower(out, "filehandle isn't readable!");
		return out;
	}
	PerlIO* Handle::out_handle() {
		PerlIO* out = IoOFP(handle);
		thrower(out, "filehandle isn't writable!");
		return out;
	}
	void Handle::print(const Scalar::Base& arg) {
		bool ret = Perl_do_print(interp, arg.get_SV(true), out_handle());
		thrower(ret, "Could not print!");
	}
	void Handle::print(int val) {
		SV* tmp = newSViv(val);
		bool ret = Perl_do_print(interp, tmp, out_handle());
		SvREFCNT_dec(tmp);
		thrower(ret, "Could not print!");
	}
	void Handle::print(Raw_string val) {
		SV* tmp = newSVpvn(val.value, val.length);
		bool ret = Perl_do_print(interp, tmp, out_handle());
		SvREFCNT_dec(tmp);
		thrower(ret, "Could not print!");
	}
	void Handle::print(const Array::Value& array) {
		for (unsigned i = 0; i < array.length(); i++) {
			print(array[i]);
		}
	}
	bool Handle::close() {
		bool ret = Perl_io_close(interp, handle, true);
		thrower(ret, "Could not close!");
		IoLINES(handle) = 0;
		IoPAGE(handle) = 0;
		IoLINES_LEFT(handle) = IoPAGE_LEN(handle);
		IoTYPE(handle) = IoTYPE_CLOSED;
		return ret;
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
