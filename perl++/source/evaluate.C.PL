#! /usr/bin/perl -T

use strict;
use warnings;

local $\ = "\n";
print q(#include "perl++/perl++.h");
print q(namespace perl {);
print qq(\tnamespace implementation{);
print qq(\t\tconst char to_eval[] = );
while (<DATA>) {
	chomp;
	next if length == 0;
	s/\\/\\\\/g;
	s/"/\\"/g;
	print qq(\t\t"$_\\n");
}
print qq(\t\t;);
print qq(\t});
print q(});
__DATA__
package Embed::Perlpp;
use strict;
use warnings;

sub regexp {
	my ($reg, $flags) = @_;
	$flags =~ / ^ [imsx]* -? [imsx]* $ /xms or die "No such flag '$flags' for qr\n";
	return $flags ? qr/(?$flags)$reg/ : qr/$reg/;
}

sub match {
	my ($string, $regex, $flags) = @_;
	die "invalid flag" if $flags =~ /[^msixpogc]/;
	my @ret = eval "\$string =~ m/\$regex/$flags";
	die $@ if $@;
	return @ret;
}

sub substitute {
	my (undef, $regex, $replacement, $flags) = @_;
	die "invalid flag" if $flags =~ /[^msixpogce]/;
	$replacement =~ s{([/])}{\\$1}g;
	my @ret = eval "\$_[0] =~ s/\$regex/$replacement/$flags";
	die $@ if $@;
	return @ret;
}
