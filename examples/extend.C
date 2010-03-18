#include "perl++.h"
#include "extend.h"

using namespace perl;

static void exporter(Interpreter& interp, const Ref<Code>& code) {
	interp.eval("print 'Success!\n'");
}

EXPORTER(exporter);

