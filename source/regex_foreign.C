#define PERL_EXT
#include "internal.h"
#include "perl++.h"
#include "regex_impl.h"

#ifndef rxres_save
#define rxres_save(a,b)      Perl_rxres_save(aTHX_ a, b)
#endif

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

		int Call_stack::subst(REGEXP* rx, SV* TARG, register SV* dstr, IV pmflags) {
			SV** base = SP;
			register char *s;
			char *strend;
			register char *m;
			const char *c;
			register char *d;
			STRLEN clen;
			I32 iters = 0;
			I32 maxiters;
			register I32 i;
			bool once;
			bool rxtainted;
			char *orig;
			I32 r_flags;
			STRLEN len;
			int force_on_match = 0;
			const I32 oldsave = PL_savestack_ix;
			STRLEN slen;
			bool doutf8 = FALSE;
#ifdef PERL_OLD_COPY_ON_WRITE
			bool is_cow;
#endif
			SV *nsv = NULL;

			/* known replacement string? */
			dstr = (pmflags & PMf_CONST) ? POPs : NULL;

#ifdef PERL_OLD_COPY_ON_WRITE
			/* Awooga. Awooga. "bool" types that are actually char are dangerous,
			   because they make integers such as 256 "false".  */
			is_cow = SvIsCOW(TARG) ? TRUE : FALSE;
#else
			if (SvIsCOW(TARG))
				sv_force_normal_flags(TARG,0);
#endif
			if (
#ifdef PERL_OLD_COPY_ON_WRITE
			!is_cow &&
#endif
				 (SvREADONLY(TARG) || ( ((SvTYPE(TARG) == SVt_PVGV && isGV_with_GP(TARG)) || SvTYPE(TARG) > SVt_PVLV) && !(SvTYPE(TARG) == SVt_PVGV && SvFAKE(TARG)))))
				Perl_die(aTHX_ PL_no_modify);
			PUTBACK;

			s = SvPV_mutable(TARG, len);
			if (!SvPOKp(TARG) || SvTYPE(TARG) == SVt_PVGV)
				force_on_match = 1;
			rxtainted = ((rx->extflags & RXf_TAINTED) || (PL_tainted && (pmflags & PMf_RETAINT)));
			if (PL_tainted)
				rxtainted |= 2;
			TAINT_NOT;

			RX_MATCH_UTF8_set(rx, DO_UTF8(TARG));

		  force_it:
			if (!s)
				Perl_die(aTHX_ "panic: pp_subst");

			strend = s + len;
			slen = RX_MATCH_UTF8(rx) ? utf8_length((U8*)s, (U8*)strend) : len;
			maxiters = 2 * slen + 10;	/* We can match twice at each
						   position, once with zero-length,
						   second time with non-zero. */

			r_flags = (rx->nparens || SvTEMP(TARG) || PL_sawampersand
				|| (rx->extflags & (RXf_EVAL_SEEN|RXf_PMf_KEEPCOPY)) )
				   ? REXEC_COPY_STR : 0;
			if (SvSCREAM(TARG))
				r_flags |= REXEC_SCREAM;

			orig = m = s;
			if (rx->extflags & RXf_USE_INTUIT) {
				PL_bostr = orig;
				s = CALLREG_INTUIT_START(rx, TARG, s, strend, r_flags, NULL);

			if (!s)
				goto nope;
			/* How to do it in subst? */
			/*	if ( (rx->extflags & RXf_CHECK_ALL)
					 && !PL_sawampersand
					 && !(rx->extflags & RXf_KEEPCOPY)
					 && ((rx->extflags & RXf_NOSCAN)
					 || !((rx->extflags & RXf_INTUIT_TAIL)
						  && (r_flags & REXEC_SCREAM))))
					goto yup;
			*/
			}

			/* only replace once? */
			once = !(pmflags & PMf_GLOBAL);

			/* known replacement string? */
			if (dstr) {
				/* replacement needing upgrading? */
				if (DO_UTF8(TARG) && !doutf8) {
					nsv = sv_newmortal();
					SvSetSV(nsv, dstr);
					if (PL_encoding)
						sv_recode_to_utf8(nsv, PL_encoding);
					else
						sv_utf8_upgrade(nsv);
					c = SvPV_const(nsv, clen);
					doutf8 = TRUE;
				}
				else {
					c = SvPV_const(dstr, clen);
					doutf8 = DO_UTF8(dstr);
				}
			}
			else {
				c = NULL;
				doutf8 = FALSE;
			}
			
			/* can do inplace substitution? */
			if (c
#ifdef PERL_OLD_COPY_ON_WRITE
			&& !is_cow
#endif
			&& (I32)clen <= rx->minlenret && (once || !(r_flags & REXEC_COPY_STR))
			&& !(rx->extflags & RXf_LOOKBEHIND_SEEN)
			&& (!doutf8 || SvUTF8(TARG))) {
				if (!CALLREGEXEC(rx, s, strend, orig, 0, TARG, NULL, r_flags | REXEC_CHECKED)) {
					SPAGAIN;
					PUSHs(&PL_sv_no);
					LEAVE_SCOPE(oldsave);
					RETURN;
				}
#ifdef PERL_OLD_COPY_ON_WRITE
				if (SvIsCOW(TARG)) {
					assert (!force_on_match);
					goto have_a_cow;
				}
#endif
				if (force_on_match) {
					force_on_match = 0;
					s = SvPV_force(TARG, len);
					goto force_it;
				}
				d = s;
				SvSCREAM_off(TARG);	/* disable possible screamer */
				if (once) {
					rxtainted |= RX_MATCH_TAINTED(rx);
					m = orig + rx->offs[0].start;
					d = orig + rx->offs[0].end;
					s = orig;
					if (m - s > strend - d) {  /* faster to shorten from end */
						if (clen) {
							Copy(c, m, clen, char);
							m += clen;
						}
						i = strend - d;
						if (i > 0) {
							Move(d, m, i, char);
							m += i;
						}
						*m = '\0';
						SvCUR_set(TARG, m - s);
					}
					else if ((i = m - s)) {	/* faster from front */
						d -= clen;
						m = d;
						sv_chop(TARG, d-i);
						s += i;
						while (i--)
							*--d = *--s;
						if (clen)
							Copy(c, m, clen, char);
					}
					else if (clen) {
						d -= clen;
						sv_chop(TARG, d);
						Copy(c, d, clen, char);
					}
					else {
						sv_chop(TARG, d);
					}
					TAINT_IF(rxtainted & 1);
					SPAGAIN;
					PUSHs(&PL_sv_yes);
				}
				else {
					do {
						if (iters++ > maxiters)
							Perl_die(aTHX_ "Substitution loop");
						rxtainted |= RX_MATCH_TAINTED(rx);
						m = rx->offs[0].start + orig;
						if ((i = m - s)) {
							if (s != d)
								Move(s, d, i, char);
							d += i;
						}
						if (clen) {
							Copy(c, d, clen, char);
							d += clen;
						}
						s = rx->offs[0].end + orig;
					} while (CALLREGEXEC(rx, s, strend, orig, s == m,
							 TARG, NULL,
							 /* don't match same null twice */
							 REXEC_NOT_FIRST|REXEC_IGNOREPOS));
					if (s != d) {
						i = strend - s;
						SvCUR_set(TARG, d - SvPVX_const(TARG) + i);
						Move(s, d, i+1, char);		/* include the NUL */
					}
					TAINT_IF(rxtainted & 1);
					SPAGAIN;
					PUSHs(sv_2mortal(newSViv((I32)iters)));
				}
				(void)SvPOK_only_UTF8(TARG);
				TAINT_IF(rxtainted);
				if (SvSMAGICAL(TARG)) {
					PUTBACK;
					mg_set(TARG);
					SPAGAIN;
				}
				SvTAINT(TARG);
				if (doutf8)
					SvUTF8_on(TARG);
				LEAVE_SCOPE(oldsave);
				RETURN;
			}

			if (CALLREGEXEC(rx, s, strend, orig, 0, TARG, NULL, r_flags | REXEC_CHECKED)) {
				if (force_on_match) {
					force_on_match = 0;
					s = SvPV_force(TARG, len);
					goto force_it;
				}
#ifdef PERL_OLD_COPY_ON_WRITE
				  have_a_cow:
#endif
				rxtainted |= RX_MATCH_TAINTED(rx);
				dstr = newSVpvn(m, s-m);
				SAVEFREESV(dstr);
				if (DO_UTF8(TARG))
					SvUTF8_on(dstr);
				if (!c) {
					register PERL_CONTEXT *cx;
					SPAGAIN;
					PUSHSUBST(cx);
					Perl_die(aTHX_ "This route is not implemented yet");
//					RETURNOP(cPMOP->op_pmreplrootu.op_pmreplroot);//XXX
				}
				r_flags |= REXEC_IGNOREPOS | REXEC_NOT_FIRST;
				do {
					if (iters++ > maxiters)
						Perl_die(aTHX_ "Substitution loop");
					rxtainted |= RX_MATCH_TAINTED(rx);
					if (RX_MATCH_COPIED(rx) && rx->subbeg != orig) {
						m = s;
						s = orig;
						orig = rx->subbeg;
						s = orig + (m - s);
						strend = s + (strend - m);
					}
					m = rx->offs[0].start + orig;
					if (doutf8 && !SvUTF8(dstr))
						sv_catpvn_utf8_upgrade(dstr, s, m - s, nsv);
					else
						sv_catpvn(dstr, s, m-s);
					s = rx->offs[0].end + orig;
					if (clen)
						sv_catpvn(dstr, c, clen);
					if (once)
						break;
				} while (CALLREGEXEC(rx, s, strend, orig, s == m,
							 TARG, NULL, r_flags));
				if (doutf8 && !DO_UTF8(TARG))
					sv_catpvn_utf8_upgrade(dstr, s, strend - s, nsv);
				else
					sv_catpvn(dstr, s, strend - s);

#ifdef PERL_OLD_COPY_ON_WRITE
				/* The match may make the string COW. If so, brilliant, because that's
				   just saved us one malloc, copy and free - the regexp has donated
				   the old buffer, and we malloc an entirely new one, rather than the
				   regexp malloc()ing a buffer and copying our original, only for
				   us to throw it away here during the substitution.  */
				if (SvIsCOW(TARG)) {
					sv_force_normal_flags(TARG, SV_COW_DROP_PV);
				} else
#endif
				{
					SvPV_free(TARG);
				}
				SvPV_set(TARG, SvPVX(dstr));
				SvCUR_set(TARG, SvCUR(dstr));
				SvLEN_set(TARG, SvLEN(dstr));
				doutf8 |= DO_UTF8(dstr);
				SvPV_set(dstr, NULL);

				TAINT_IF(rxtainted & 1);
				SPAGAIN;
				PUSHs(sv_2mortal(newSViv((I32)iters)));

				(void)SvPOK_only(TARG);
				if (doutf8)
					SvUTF8_on(TARG);
				TAINT_IF(rxtainted);
				SvSETMAGIC(TARG);
				SvTAINT(TARG);
				LEAVE_SCOPE(oldsave);
				RETURN;
			}
			goto ret_no;

			nope:
			ret_no:
			SPAGAIN;
			PUSHs(&PL_sv_no);
			LEAVE_SCOPE(oldsave);
			RETURN;
		}
	}
}
