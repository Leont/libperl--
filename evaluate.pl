#! /usr/bin/perl

use strict;
use warnings;

local $\ = "\n";
print qq(#include "perl++.h");
print qq(namespace perl {);
print qq(	namespace implementation{);
print qq(		const char to_eval[] = );
while (<DATA>) {
	chomp;
	next if length == 0;
	s/\\/\\\\/g;
	s/"/\\"/g;
	print qq(		"$_\\n");
}
print qq(		;);
print qq(	});
print qq(});
__DATA__
package Embed::Perlpp;
use strict;
use warnings;

sub regexp {
	my $reg = shift;
	return qr/$reg/;
}

sub match {
	my ($string, $regex) = @_;
	return $string =~ /$regex/;
}

sub substitute {
	my (undef, $regex, $replacement) = @_;
	$replacement =~ s{/}{\/}g;
	my $ret = eval "\$_[0] =~ s/\$regex/$replacement/";
	die $@ if $@;
	return $ret;
}
