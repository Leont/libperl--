package Library::Build::Config;

use 5.006;
use strict;
use warnings FATAL => 'all';

our $VERSION = '0.003';

use Carp qw/croak/;
use File::Spec::Functions qw/catfile/;
use Readonly ();

Readonly::Scalar my $NOTFOUND => -1;

sub parse_line {
	my ($action, $line) = @_;

	for ($line) {
		next if / ^ \# /xms;
		s{ \n \s+ }{ }x;
		next if / ^ \s* $ /xms;


		if (my ($name, $args) = m/ \A \s* (\* | \w+ ) \s+ (.*?) \s* \z /xms) {
			my @args = split / \s+ /x, $args;
			return @args if $name eq $action or $name eq '*';
		}
		else {
			croak "Can't parse line '$_'";
		}
	}
	return;
}

sub read_config {
	my $action = shift;

	my @ret;

	my @files = (
		($ENV{MODULEBUILDRC} ? $ENV{MODULEBUILDRC}                        : ()),
		($ENV{HOME} ?          catfile($ENV{HOME},'.modulebuildrc')       : ()),
		($ENV{USERPROFILE} ?   catfile($ENV{USERPROFILE},'.modulebuldrc') : ()),
	);
	FILE:
	for my $file (@files) {
		next FILE if not -e $file;

		open my $fh, '<', $file or croak "Couldn't open configuration file '$file': $!";
		my @lines = split / \n (?! \s) /xms, do { local $/ = undef, <$fh> };
		close $fh or croak "Can't close $file: $!";
		for my $line (@lines) {
			push @ret, parse_line($action, $line);
		}
	}
	return @ret;
}

1;
