package Library::Build::Args;

use 5.006;
use strict;
use warnings FATAL => 'all';

use Carp qw/croak/;
use File::Spec::Functions qw/catfile/;
use List::MoreUtils qw/first_index/;
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

sub parse_action {
	my $meta_arguments = shift;
	for my $meta_argument (map { $meta_arguments->{$_} } qw/argv envs/) {
		my $position = first_index { not m/ ^ -- /xms and not m/ = /xms } @{$meta_argument};
		return splice @{$meta_argument}, $position, 1 if $position != $NOTFOUND;
	}
	return;
}

sub set_option {
	my ($options, $key, $value) = @_;
	if ($key eq 'verbose') {
		return $options->{quiet} = -$value;
	}
	else {
		return $options->{$key} = $value;
	}
}

sub parse_option {
	my ($options, $argument) = @_;
	$argument =~ s/ ^ -- //xms;
	if ($argument =~ / \A (\w+) = (.*) \z /xms) {
		set_option($options, $1, $2);
	}
	else {
		set_option($options, $argument, 1);
	}
	return;
}

sub parse_options {
	my %meta_arguments = @_;
	@{ $meta_arguments{envs} } = split / /, $ENV{PERL_MB_OPT} if not defined $meta_arguments{envs} and $ENV{PERL_MB_OPT};

	my %options = ( quiet => 0 );

	$options{action} = parse_action(\%meta_arguments);

	@{ $meta_arguments{config} } = read_config($options{action} || 'build');

	for my $argument_list (map { $meta_arguments{$_} } qw/config cached envs argv/) {
		for my $argument (@{$argument_list}) {
			parse_option(\%options, $argument);
		}
	}
	return %options;
}

1;
