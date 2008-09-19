#include "perl++.h"
#include "tap++.h"

using namespace perl;

int main(int argc, char** argv) {
	TEST_START(5);
	FAIL(throw Runtime_exception("Runtime exception"), "Should throw a Runtime exception");
	FAIL(assertion<Runtime_exception>(false, "Runtime exception"), "Should throw a Runtime exception too");
	TRY(assertion<Runtime_exception>(true, "Runtime exception"), "Shouldn't throw a Runtime exception");

	Raw_string test = "Test";
	is(test, "Test", Raw_string("Raw_strings equal equal strings"));
	is(test, "Test", "Raw_strings equal equal const char*");
	diag("TODO: more test on Raw_strings");
	TEST_END;
}
