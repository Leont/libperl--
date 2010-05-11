namespace perl {
	namespace implementation { 
		class Regex {
			interpreter* interp;
			friend class perl::Regex;
			SV* pattern;
			public:
			Regex(interpreter*, SV*);
			SV* get_SV() const {
				return pattern;
			}
		};
	}
}
