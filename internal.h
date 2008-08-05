/*
 * This is an internal header and should probably not be included in your programs
 */
#define PERL_NO_GET_CONTEXT
#include <iostream>
#include <EXTERN.h>
#include <perl.h>
//#include <perlio.h>
#ifdef INCLUDE_XSUB
#include <XSUB.h>
#endif
#include "ppport.h"

#undef aTHX
#define aTHX interp
#undef call_method
#undef die
