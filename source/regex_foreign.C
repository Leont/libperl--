#include "internal.h"
#include "perl++.h"
#include "regex_impl.h"

#undef RETURN
#define RETURN do { PUTBACK; return SP - base; } while(0)
#undef RETURNX
#define RETURNX(X) do { X; PUTBACK; return SP - base; } while(0)
#define my_perl interp

namespace perl {
	namespace implementation {
		int Call_stack::match(REGEXP* rx, SV* TARG, IV gimme, IV pmflags) {
			SV** base = SP;
			register const char *t;
			register const char *s;
			const char *strend;
			I32 global;
			I32 r_flags = REXEC_CHECKED;
			const char *truebase;			/* Start of string  */
			bool rxtainted;
			STRLEN len;
			I32 minmatch = 0;
			const I32 oldsave = PL_savestack_ix;
			I32 update_minmatch = 1;
			I32 had_zerolen = 0;
			U32 gpos = 0;

			s = SvPV_const(TARG, len);
			if (!s)
				Perl_die(aTHX_ "panic: pp_match");
			strend = s + len;
			rxtainted = ((rx->extflags & RXf_TAINTED) ||
				 (PL_tainted && (pmflags & PMf_RETAINT)));
			TAINT_NOT;

			RX_MATCH_UTF8_set(rx, DO_UTF8(TARG));

			if (rx->minlen > (I32)len) {
				if (gimme == G_ARRAY)
					RETURN;
				RETPUSHNO;
			}
			
			truebase = t = s;

			/* XXXX What part of this is needed with true \G-support? */
			if ((global = pmflags & PMf_GLOBAL)) {
				rx->offs[0].start = -1;
				if (SvTYPE(TARG) >= SVt_PVMG && SvMAGIC(TARG)) {
					MAGIC* const mg = mg_find(TARG, PERL_MAGIC_regex_global);
					if (mg && mg->mg_len >= 0) {
					if (!(rx->extflags & RXf_GPOS_SEEN))
						rx->offs[0].end = rx->offs[0].start = mg->mg_len;
					else if (rx->extflags & RXf_ANCH_GPOS) {
						r_flags |= REXEC_IGNOREPOS;
						rx->offs[0].end = rx->offs[0].start = mg->mg_len;
					} else if (rx->extflags & RXf_GPOS_FLOAT) 
						gpos = mg->mg_len;
					else 
						rx->offs[0].end = rx->offs[0].start = mg->mg_len;
					minmatch = (mg->mg_flags & MGf_MINMATCH) ? rx->gofs + 1 : 0;
					update_minmatch = 0;
					}
				}
			}

			/* XXX: comment out !global get safe $1 vars after a
			   match, BUT be aware that this leads to dramatic slowdowns on
			   /g matches against large strings.  So far a solution to this problem
			   appears to be quite tricky.
			   Test for the unsafe vars are TODO for now. */
			if ((  !global &&  rx->nparens) 
				|| SvTEMP(TARG) || PL_sawampersand ||
				(rx->extflags & (RXf_EVAL_SEEN|RXf_PMf_KEEPCOPY)))
			r_flags |= REXEC_COPY_STR;
			if (SvSCREAM(TARG))
				r_flags |= REXEC_SCREAM;
			
		play_it_again:
			if (global && rx->offs[0].start != -1) {
					t = s = rx->offs[0].end + truebase - rx->gofs;
				if ((s + rx->minlen) > strend || s < truebase)
					goto nope;
				if (update_minmatch++)
					minmatch = had_zerolen;
			}
			if (rx->extflags & RXf_USE_INTUIT &&
				DO_UTF8(TARG) == ((rx->extflags & RXf_UTF8) != 0)) {
				/* FIXME - can PL_bostr be made const char *?  */
				PL_bostr = (char *)truebase;
				s = CALLREG_INTUIT_START(rx, TARG, (char *)s, (char *)strend, r_flags, NULL);

				if (!s)
					goto nope;
				if ( (rx->extflags & RXf_CHECK_ALL)
					 && !PL_sawampersand
					 && !(rx->extflags & RXf_PMf_KEEPCOPY)
					 && ((rx->extflags & RXf_NOSCAN)
					 || !((rx->extflags & RXf_INTUIT_TAIL)
						  && (r_flags & REXEC_SCREAM)))
					 && !SvROK(TARG))	/* Cannot trust since INTUIT cannot guess ^ */
					goto yup;
			}

			if (CALLREGEXEC(rx, (char*)s, (char *)strend, (char*)truebase, minmatch, TARG, INT2PTR(void*, gpos), r_flags)) {
				goto gotcha;
			}
			else
				goto ret_no;
			/*NOTREACHED*/

		  gotcha:
			if (rxtainted)
				RX_MATCH_TAINTED_on(rx);
			TAINT_IF(RX_MATCH_TAINTED(rx));
			if (gimme == G_ARRAY) {
				const I32 nparens = rx->nparens;
				I32 i = (global && !nparens) ? 1 : 0;

				SPAGAIN;			/* EVAL blocks could move the stack. */
				EXTEND(SP, nparens + i);
				EXTEND_MORTAL(nparens + i);
				for (i = !i; i <= nparens; i++) {
					PUSHs(sv_newmortal());
					if ((rx->offs[i].start != -1) && rx->offs[i].end != -1 ) {
						const I32 len = rx->offs[i].end - rx->offs[i].start;
						s = rx->offs[i].start + truebase;
							if (rx->offs[i].end < 0 || rx->offs[i].start < 0 || len < 0 || len > strend - s)
								Perl_die(aTHX_ "panic: pp_match start/end pointers");
						sv_setpvn(*SP, s, len);
						if (DO_UTF8(TARG) && is_utf8_string((U8*)s, len))
							SvUTF8_on(*SP);
					}
				}
				if (global) {
					if (pmflags & PMf_CONTINUE) {
						MAGIC* mg = NULL;
						if (SvTYPE(TARG) >= SVt_PVMG && SvMAGIC(TARG))
							mg = mg_find(TARG, PERL_MAGIC_regex_global);
						if (!mg) {
#ifdef PERL_OLD_COPY_ON_WRITE
							if (SvIsCOW(TARG))
								sv_force_normal_flags(TARG, 0);
#endif
							mg = sv_magicext(TARG, NULL, PERL_MAGIC_regex_global,
									 &PL_vtbl_mglob, NULL, 0);
						}
						if (rx->offs[0].start != -1) {
							mg->mg_len = rx->offs[0].end;
							if (rx->offs[0].start + rx->gofs == (UV)rx->offs[0].end)
								mg->mg_flags |= MGf_MINMATCH;
							else
								mg->mg_flags &= ~MGf_MINMATCH;
						}
					}
					had_zerolen = (rx->offs[0].start != -1
						   && (rx->offs[0].start + rx->gofs
							   == (UV)rx->offs[0].end));
					PUTBACK;			/* EVAL blocks may use stack */
					r_flags |= REXEC_IGNOREPOS | REXEC_NOT_FIRST;
					goto play_it_again;
				}
				else if (!nparens)
					XPUSHs(&PL_sv_yes);
				LEAVE_SCOPE(oldsave);
				RETURN;
			}
			else {
				if (global) {
					MAGIC* mg;
					if (SvTYPE(TARG) >= SVt_PVMG && SvMAGIC(TARG))
						mg = mg_find(TARG, PERL_MAGIC_regex_global);
					else
						mg = NULL;
					if (!mg) {
#ifdef PERL_OLD_COPY_ON_WRITE
						if (SvIsCOW(TARG))
							sv_force_normal_flags(TARG, 0);
#endif
						mg = sv_magicext(TARG, NULL, PERL_MAGIC_regex_global,
								 &PL_vtbl_mglob, NULL, 0);
					}
					if (rx->offs[0].start != -1) {
						mg->mg_len = rx->offs[0].end;
						if (rx->offs[0].start + rx->gofs == (UV)rx->offs[0].end)
							mg->mg_flags |= MGf_MINMATCH;
						else
							mg->mg_flags &= ~MGf_MINMATCH;
					}
				}
				LEAVE_SCOPE(oldsave);
				RETPUSHYES;
			}

		yup:					/* Confirmed by INTUIT */
			if (rxtainted)
				RX_MATCH_TAINTED_on(rx);
			TAINT_IF(RX_MATCH_TAINTED(rx));
			if (RX_MATCH_COPIED(rx))
				Safefree(rx->subbeg);
			RX_MATCH_COPIED_off(rx);
			rx->subbeg = NULL;
			if (global) {
				/* FIXME - should rx->subbeg be const char *?  */
				rx->subbeg = (char *) truebase;
				rx->offs[0].start = s - truebase;
				if (RX_MATCH_UTF8(rx)) {
					char * const t = (char*)utf8_hop((U8*)s, rx->minlenret);
					rx->offs[0].end = t - truebase;
				}
				else {
					rx->offs[0].end = s - truebase + rx->minlenret;
				}
				rx->sublen = strend - truebase;
				goto gotcha;
			}
			if (PL_sawampersand || rx->extflags & RXf_PMf_KEEPCOPY) {
				I32 off;
#ifdef PERL_OLD_COPY_ON_WRITE
				if (SvIsCOW(TARG) || (SvFLAGS(TARG) & CAN_COW_MASK) == CAN_COW_FLAGS) {
					if (DEBUG_C_TEST) {
					PerlIO_printf(Perl_debug_log,
							  "Copy on write: pp_match $& capture, type %d, truebase=%p, t=%p, difference %d\n",
							  (int) SvTYPE(TARG), (void*)truebase, (void*)t,
							  (int)(t-truebase));
					}
					rx->saved_copy = sv_setsv_cow(rx->saved_copy, TARG);
					rx->subbeg = (char *) SvPVX_const(rx->saved_copy) + (t - truebase);
					assert (SvPOKp(rx->saved_copy));
				} else
#endif
				{

					rx->subbeg = savepvn(t, strend - t);
#ifdef PERL_OLD_COPY_ON_WRITE
					rx->saved_copy = NULL;
#endif
				}
				rx->sublen = strend - t;
				RX_MATCH_COPIED_on(rx);
				off = rx->offs[0].start = s - t;
				rx->offs[0].end = off + rx->minlenret;
			}
			else {			/* startp/endp are used by @- @+. */
				rx->offs[0].start = s - truebase;
				rx->offs[0].end = s - truebase + rx->minlenret;
			}
			/* including rx->nparens in the below code seems highly suspicious.
			   -dmq */
			rx->nparens = rx->lastparen = rx->lastcloseparen = 0;	/* used by @-, @+, and $^N */
			LEAVE_SCOPE(oldsave);
			RETPUSHYES;


		nope:
		ret_no:
			if (global && !(pmflags & PMf_CONTINUE)) {
				if (SvTYPE(TARG) >= SVt_PVMG && SvMAGIC(TARG)) {
					MAGIC* const mg = mg_find(TARG, PERL_MAGIC_regex_global);
					if (mg)
						mg->mg_len = -1;
				}

			}
			LEAVE_SCOPE(oldsave);
			if (gimme == G_ARRAY)
				RETURN;
			RETPUSHNO;
		}

	}
}
