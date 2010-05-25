/*
 * This is an internal header and should not be included in your programs
 */
#define PERL_NO_GET_CONTEXT
#include <EXTERN.h>
#include <perl.h>

#undef do_open
#undef do_close
//#include <perlio.h>
//#ifdef INCLUDE_XSUB
//#include <XSUB.h>
//#endif

#undef aTHX
#define aTHX interp
#undef die

#ifdef MGf_LOCAL
#define MAGIC_TAIL ,0, 0, 0
#else
#define MAGIC TAIL 
#endif
#undef call_list
