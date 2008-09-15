#! /usr/bin/perl

use strict;
use warnings;

local $\ = "\n";
print qq(#include "perl++.h");
print qq(namespace perl {);
print qq(	namespace implementation{);
print qq(		const char to_eval[] = );
while (my $line = <DATA>) {
	chomp $line;
	next if length($line) == 0;
	$line =~ s/\\/\\\\/g;
	$line =~ s/"/\\"/g;
	print qq(		"$line");
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
	my ($string, $regex, $replacement) = @_;
	return $string =~ s/$regex/$replacement/e;
}

sub substitutee {
	my ($string, $regex, $replacement) = @_;
	return $string =~ s/$regex/$replacement/ee;
}
