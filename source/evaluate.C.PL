#! /usr/bin/perl -T

use strict;
use warnings;

local $\ = "\n";
print q(#include "perl++.h");
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
	for ($string) {
		return $flags eq ''   ? m/$regex/ 
			:  $flags eq 'g'  ? m/$regex/g
			:  $flags eq 'gc' ? m/$regex/gc
			:  die "Unknown flag or flag combination '$flags'"
	}
}

sub substitute {
	my (undef, $regex, $replacement, $flags) = @_;
	for ($_[0]) {
		return $flags eq ''   ? s/$regex/$replacement/
			:  $flags eq 'g'  ? s/$regex/$replacement/g
			:  $flags eq 'gc' ? s/$regex/$replacement/gc
			: die "Unknown flag or flag combination '$flags'"
	}
#	$replacement =~ s{/}{\/}g;
#	my $ret = eval "\$_[0] =~ s/\$regex/$replacement/$flags";
#	die $@ if $@;
#	return $ret;
}
