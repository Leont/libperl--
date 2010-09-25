#include <config.h>
namespace perl {
#include <patchlevel.h>
}

namespace perl {
	typedef IVTYPE IV;
	typedef UVTYPE UV;
	typedef NVTYPE NV;

	const int revision      = PERL_REVISION;
	const int major_version = PERL_VERSION;
	const int minor_version = PERL_SUBVERSION;
}
