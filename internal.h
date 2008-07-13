/*
 * This is an internal header and should probably not be included in your programs
 */
#define PERL_NO_SHORT_NAMES
#define PERL_NO_GET_CONTEXT
#include <EXTERN.h>
#include <perl.h>
//#include <perlio.h>

#undef aTHX
#define aTHX interp

#define mg_get(a) Perl_mg_get(aTHX_ a)
