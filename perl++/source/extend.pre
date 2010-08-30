#define PERL_NO_GET_CONTEXT
#define __cplusplus
#include "XSUB.h"

#define hash #

hash ifndef pTHX_
hash   ifdef PERL_IMPLICIT_CONTEXT
hash     define pTHX_ interpreter* aTHX,
hash   endif
hash   define CV perl::CV
hash endif

hash define EXPORTER(exporter) XS(perlpp_exporter) { perl::implementation::Exporter_helper helper(aTHX); helper(exporter); }
