package Library::Build::Config;

use 5.006;
use strict;
use warnings FATAL => 'all';

our $VERSION = '0.003';

use Carp qw/croak carp/;
use File::Spec::Functions qw/catfile/;

my $NOTFOUND = -1;

my @files = (
	($ENV{MODULEBUILDRC} ? $ENV{MODULEBUILDRC}                         : ()),
	($ENV{HOME} ?          catfile($ENV{HOME}, '.modulebuildrc')       : ()),
	($ENV{USERPROFILE} ?   catfile($ENV{USERPROFILE}, '.modulebuldrc') : ()),
);

sub slurp {
	my $filename = shift;
	open my $fh, '<', $filename or croak "Couldn't open configuration file '$filename': $!";
	my $content = do { local $/ = undef, <$fh> };
	close $fh or croak "Can't close $filename: $!";
	return $content;
}

sub read_config {
	my $action = shift;
	my @ret;

	FILE:
	for my $filename (@files) {
		next FILE if not -e $filename;

		my $content = slurp($filename);

		$content =~ s/ (?<!\\) \# [^\n]*//gxm; # Remove comments
		$content =~ s/ \n [ \t\f]+ / /gx;      # Join multi-lines
		LINE:
		for my $line (split /\n/, $content) {
			next LINE if $line =~ / \A \s* \z /xms;  # Skip empty lines
			if (my ($name, $args) = $line =~ m/ \A \s* (\* | [\w.-]+ ) \s+ (.*?) \s* \z /xms) {
				push @ret, split / \s+ /x, $args if $name eq $action or $name eq '*';
			}
			else {
				carp "Can't parse line '$line'";
			}
		}
		last FILE;
	}
	return @ret;
}

1;
