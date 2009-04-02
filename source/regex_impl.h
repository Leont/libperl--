namespace perl {
	namespace implementation { 
		class Regex {
			interpreter* interp;
			friend class perl::Regex;
#if PERL_VERSION < 9
			SV* pattern;
			public:
			Regex(interpreter*, SV*);
			SV* get_SV() const {
				return pattern;
			}
#else 
			REGEXP* pattern;
			public:
			Regex(interpreter*, SV*, const char*);
			int match(const Scalar::Base&);
			SV* get_SV() const {
				return NULL;
			}
#endif
		};
#if PERL_VERSION > 8
		void match(pTHX_ REGEXP* rx, SV* TARG, I32 gimme, IV pmflags);
#endif
	}
}
