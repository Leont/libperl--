namespace perl {
	namespace implementation { 
		class Regex {
			interpreter* interp;
			SV* pattern;
			friend class perl::Regex;
			public:
			Regex(interpreter*, SV*);
			SV* get_SV() const {
				return pattern;
			}
		};
	}
}
